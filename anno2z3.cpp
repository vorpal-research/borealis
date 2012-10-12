/*
 * anno2z3.cpp
 *
 *  Created on: Oct 12, 2012
 *      Author: belyaev
 */

#include <z3/z3++.h>
#include <llvm/Value.h>

#include "Anno/anno.hpp"
#include "util.h"


namespace borealis {

class z3Visitor: public productionVisitor {

public:
    typedef std::unordered_map<std::string, llvm::Value*> name_context;
    typedef z3::context z3_context;

    typedef z3::expr expr;

private:
    const z3_context& z3;
    const name_context& names;
    expr retVal;

public:
    z3Visitor(const z3_context& z3, const name_context& names):z3(z3), names(names) {}
    virtual ~z3Visitor(){}

    const expr& get() { return retVal; }

    virtual void onDoubleConstant(double v) {
        using borealis::util::toString;
        retVal = z3.real_val(toString(v).c_str());
    }
    virtual void onIntConstant(int v) {
        retVal = z3.int_val(v);
    }
    virtual void onBoolConstant(bool v) {
        retVal = z3.bool_val(v);
    }

    virtual void onVariable(const std::string&) {
        // FIXME
    }
    virtual void onBuiltin(const std::string&) {
        // FIXME
    }
    virtual void onMask(const std::string&) {
        // do nothing, masks have nothing to do with smt
    }

    virtual void onBinary(bin_opcode op, const prod_t& lhv, const prod_t& rhv) {
        z3Visitor left(z3, names);
        z3Visitor right(z3, names);
        lhv->accept(left);
        rhv->accept(right);

        switch(op) {
        case OPCODE_PLUS:  retVal = left.retVal +  right.retVal; break;
        case OPCODE_MINUS: retVal = left.retVal -  right.retVal; break;
        case OPCODE_MULT:  retVal = left.retVal *  right.retVal; break;
        case OPCODE_DIV:   retVal = left.retVal /  right.retVal; break;
        case OPCODE_MOD:   retVal = left.retVal %  right.retVal; break;
        case OPCODE_EQ:    retVal = left.retVal == right.retVal; break;
        case OPCODE_NE:    retVal = left.retVal != right.retVal; break;
        case OPCODE_GT:    retVal = left.retVal >  right.retVal; break;
        case OPCODE_LT:    retVal = left.retVal <  right.retVal; break;
        case OPCODE_GE:    retVal = left.retVal >= right.retVal; break;
        case OPCODE_LE:    retVal = left.retVal <= right.retVal; break;
        case OPCODE_LAND:  retVal = left.retVal && right.retVal; break;
        case OPCODE_LOR:   retVal = left.retVal || right.retVal; break;
        case OPCODE_BAND:  retVal = left.retVal &  right.retVal; break;
        case OPCODE_BOR:   retVal = left.retVal |  right.retVal; break;
        case OPCODE_XOR:   retVal = left.retVal ^  right.retVal; break;
        case OPCODE_LSH:   retVal = left.retVal << right.retVal; break;
        case OPCODE_RSH:   retVal = left.retVal >> right.retVal; break;
        }

    }
    virtual void onUnary(un_opcode op, const prod_t&) {
        z3Visitor operand(z3, names);

        switch(op) {
        case OPCODE_NEG:  retVal = -operand.retVal; break;
        case OPCODE_NOT:  retVal = !operand.retVal; break;
        case OPCODE_BNOT: retVal = ~operand.retVal; break;
        }
    }

};

} // namespace borealis


