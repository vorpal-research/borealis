/*
 * anno2z3.cpp
 *
 *  Created on: Oct 12, 2012
 *      Author: belyaev
 */

#include <llvm/Value.h>

#include <z3/z3++.h>

#include <sstream>
#include <unordered_map>

#include "Anno/anno.hpp"
#include "Solver/Z3ExprFactory.h"
#include "util.h"


#pragma GCC diagnostic warning "-Wswitch-enum"


namespace borealis {

class z3Visitor: public productionVisitor {

public:

    typedef std::unordered_map<std::string, llvm::Value*> name_context;
    typedef z3::context z3_context;

    typedef z3::expr expr;

private:

    z3_context& z3;
    Z3ExprFactory z3ef;

    const name_context& names;
    std::unique_ptr<expr> retVal;

    bool fucked_up;
    std::ostringstream errors;

    void assign(const expr& exp) { retVal.reset(new expr(exp)); }

    bool ok() { return !fucked_up; }
    std::string error_message() { return errors.str(); }

public:

    z3Visitor(z3_context& z3, const name_context& names): z3(z3), z3ef(z3), names(names), retVal(nullptr), fucked_up(false) {}
    virtual ~z3Visitor(){}

    const expr& get() { return *retVal; }

    virtual void onDoubleConstant(double v) {
        if(fucked_up) return;

        assign( z3ef.getRealConst(v) );
    }
    virtual void onIntConstant(int v) {
        if(fucked_up) return;

        assign( z3ef.getIntConst(v) );
    }
    virtual void onBoolConstant(bool v) {
        if(fucked_up) return;

        assign( z3ef.getBoolConst(v) );
    }

    virtual void onVariable(const std::string& str) {
        if(fucked_up) return;

        if(!!names.count(str)) {
            auto v = names.at(str);
            assign( z3ef.getExprForValue(*v, str) );
        } else {
            fucked_up = true;
            errors << "Could not find the desired variable: " << str;
        }
    }
    virtual void onBuiltin(const std::string&) {
        if(fucked_up) return;

        fucked_up = true;
        errors  << "Builtins are not supported yet";

        // FIXME
    }
    virtual void onMask(const std::string&) {
        // do nothing, masks have nothing to do with smt
    }

    expr make_mod_expr(expr const& lhv, expr const& rhv) {
        if(lhv.is_arith() && rhv.is_arith()) {
            return expr(z3, Z3_mk_mod(z3, lhv, rhv));
        } else if(lhv.is_bv() && rhv.is_bv()){
            return expr(z3, Z3_mk_bvsrem(z3, lhv, rhv));
        } else {
            fucked_up = true;
            errors << "Used mod operation on incompatible expressions: "
                   << lhv << " and " << rhv;
            return expr(z3);
        }
    }

    expr make_lsh_expr(expr const& lhv, expr const& rhv) {
        if(lhv.is_bv() && rhv.is_bv()) {
            return expr(z3, Z3_mk_bvshl(z3, lhv, rhv));
        } else if(lhv.is_arith() && rhv.is_arith()) {
            return lhv * pw(2, rhv);
        } else {
            fucked_up = true;
            errors << "Used left shift operation on incompatible expressions: "
                   << lhv << " and " << rhv;
            return expr(z3);
        }
    }

    expr make_rsh_expr(expr const& lhv, expr const& rhv) {
        if(lhv.is_arith() && rhv.is_arith()) {
            return expr(z3, Z3_mk_bvashr(z3, lhv, rhv));
        } else if(lhv.is_bv() && rhv.is_bv()){
            return lhv / pw(2, rhv);
        } else
        {
            fucked_up = true;
            errors << "Used right shift operation on incompatible expressions: "
                   << lhv << " and " << rhv;
            return expr(z3);
        }
    }

    virtual void onBinary(bin_opcode op, const prod_t& lhv, const prod_t& rhv) {
        if(fucked_up) return;
        else {
            z3Visitor left(z3, names);
            lhv->accept(left);
            if(left.fucked_up) {
                errors << left.error_message();
                fucked_up = true;
                return;
            }

            z3Visitor right(z3, names);
            rhv->accept(right);
            if(right.fucked_up) {
                errors << right.error_message();
                fucked_up = true;
                return;
            }

            switch(op) {
            typedef bin_opcode op;
            // cases supported by Z3 api
            case op::OPCODE_PLUS:  assign( left.get() +  right.get() ); break;
            case op::OPCODE_MINUS: assign( left.get() -  right.get() ); break;
            case op::OPCODE_MULT:  assign( left.get() *  right.get() ); break;
            case op::OPCODE_DIV:   assign( left.get() /  right.get() ); break;
            case op::OPCODE_EQ:    assign( left.get() == right.get() ); break;
            case op::OPCODE_NE:    assign( left.get() != right.get() ); break;
            case op::OPCODE_GT:    assign( left.get() >  right.get() ); break;
            case op::OPCODE_LT:    assign( left.get() <  right.get() ); break;
            case op::OPCODE_GE:    assign( left.get() >= right.get() ); break;
            case op::OPCODE_LE:    assign( left.get() <= right.get() ); break;
            case op::OPCODE_LAND:  assign( left.get() && right.get() ); break;
            case op::OPCODE_LOR:   assign( left.get() || right.get() ); break;
            case op::OPCODE_BAND:  assign( left.get() &  right.get() ); break;
            case op::OPCODE_BOR:   assign( left.get() |  right.get() ); break;
            case op::OPCODE_XOR:   assign( left.get() ^  right.get() ); break;
            // cases not supported:
            case op::OPCODE_MOD:   assign( make_mod_expr(left.get(), right.get()) ); break;
            case op::OPCODE_LSH:   assign( make_lsh_expr(left.get(), right.get()) ); break;
            case op::OPCODE_RSH:   assign( make_rsh_expr(left.get(), right.get()) ); break;
            }
        }
    }

    virtual void onUnary(un_opcode op, const prod_t& p) {
        if(fucked_up) return;
        else {
            z3Visitor operand(z3, names);
            p->accept(operand);
            if(operand.fucked_up) {
                errors << operand.error_message();
                fucked_up = true;
                return;
            }

            switch(op) {
            typedef un_opcode op;
            case op::OPCODE_NEG:  assign( -operand.get() ); break;
            case op::OPCODE_NOT:  assign( !operand.get() ); break;
            case op::OPCODE_BNOT: assign( ~operand.get() ); break;
            }
        }
    }

};

} // namespace borealis
