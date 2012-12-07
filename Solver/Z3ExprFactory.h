/*
 * Z3ExprFactory.h
 *
 *  Created on: Oct 30, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3EXPRFACTORY_H_
#define Z3EXPRFACTORY_H_

#include <llvm/Target/TargetData.h>
#include <z3/z3++.h>

#include "Term/Term.h"
#include "Util/util.h"

#include "Logging/tracer.hpp"

namespace borealis {

class Z3ExprFactory {

public:
    typedef z3::func_decl function;
    typedef function array;
    typedef z3::expr expr;
    typedef z3::sort sort;
    typedef std::vector<expr> expr_vector;

    // a hack: CopyAssignable reference to non-CopyAssignable object
    // (z3::expr is CopyConstructible, but not CopyAssignable, so no
    // accumulator-like shit is possible with it)
    typedef borealis::util::copyref<expr> exprRef;

    Z3ExprFactory(z3::context& ctx);

    z3::context& unwrap() {
        return ctx;
    }

    ////////////////////////////////////////////////////////////////////////////

    expr getPtr(const std::string& name);
    expr getNullPtr();
    expr getBoolVar(const std::string& name);
    expr getBoolConst(bool v);
    expr getBoolConst(const std::string& v);
    expr getTrue();
    expr getFalse();
    expr getIntVar(const std::string& name, size_t bits);
    expr getFreshIntVar(const std::string& name, size_t bits);
    expr getIntConst(int v, size_t bits);
    expr getIntConst(const std::string& v, size_t bits);
    expr getRealVar(const std::string& name);
    expr getFreshRealVar(const std::string& name);
    expr getRealConst(int v);
    expr getRealConst(double v);
    expr getRealConst(const std::string& v);
    array getNoMemoryArray();
    expr getNoMemoryArrayAxiom(array mem);
    expr_vector splitBytes(expr bv);
    expr concatBytes(const expr_vector& bytes);
    expr getForAll(
        const std::vector<sort>& sorts,
        std::function<expr(const expr_vector&)> func
    );
    function createFreshFunction(const std::string& name, const std::vector<sort>& domain, sort range);
    std::pair<array, expr> byteArrayInsert(array arr, expr ix, expr bv);
    expr byteArrayExtract(array arr, expr ix, unsigned sz);
    expr getExprForTerm(const Term& term, size_t bits = 0);
    expr getExprForValue(
            const llvm::Value& value,
            const std::string& name);
    expr getInvalidPtr();
    expr isInvalidPtrExpr(expr ptr);
    expr getDistinct(const expr_vector& exprs);

    struct then_tmp {
        expr cmd;
        expr branch;

        expr else_(expr elsebranch) {
            return z3::to_expr(cmd.ctx(), Z3_mk_ite(cmd.ctx(), cmd, branch, elsebranch));
        }
    };

    struct if_tmp {
        expr cond;

        then_tmp then_(expr branch) {
            return then_tmp{ cond, branch };
        }
    };

    if_tmp if_(expr cond) {
        return if_tmp{ cond };
    }

    expr switch_(
            expr val,
            const std::vector<std::pair<expr, expr>>& cases,
            expr default_);

private:

    expr to_expr(Z3_ast ast);
    expr getExprByTypeAndName(
            const llvm::ValueType type,
            const std::string& name,
            size_t bitsize = 0);

public:

    ////////////////////////////////////////////////////////////////////////////

    function getDerefFunction(sort& domain, sort& range);
    function getGEPFunction();

    ////////////////////////////////////////////////////////////////////////////

    sort getPtrSort();

    ////////////////////////////////////////////////////////////////////////////

    static void initialize(llvm::TargetData* TD);

private:

    z3::context& ctx;

    static unsigned int pointerSize;

};

} /* namespace borealis */

#endif /* Z3EXPRFACTORY_H_ */
