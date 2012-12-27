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
#include "Logic.hpp"

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

    typedef borealis::logic::Bool Bool;
    typedef borealis::logic::BitVector<32> Pointer;
    typedef borealis::logic::BitVector<Pointer::bitsize> Integer;
    typedef borealis::logic::BitVector<Pointer::bitsize> Real;
    typedef borealis::logic::BitVector<Pointer::bitsize> Byte;
    typedef borealis::logic::ScatterArray<Pointer, Byte::bitsize> MemArray;
    typedef borealis::logic::SomeExpr Dynamic;

    Z3ExprFactory(z3::context& ctx);

    z3::context& unwrap() {
        return ctx;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Pointers
    Pointer getPtr(const std::string& name);
    Pointer getNullPtr();
    // Bools
    Bool getBoolVar(const std::string& name);
    Bool getBoolConst(bool b);
    Bool getTrue();
    Bool getFalse();
    // Integers
    Integer getIntVar(const std::string& name, size_t bits);
    Integer getFreshIntVar(const std::string& name, size_t bits);
    Integer getIntConst(int v, size_t bits);
    Integer getIntConst(const std::string& v, size_t bits);
    // Reals
    Real getRealVar(const std::string& name);
    Real getFreshRealVar(const std::string& name);
    Real getRealConst(int v);
    Real getRealConst(double v);
    Real getRealConst(const std::string& v);
    // Memory
    MemArray getNoMemoryArray();

    // Generic functions
    Dynamic getExprForTerm(const Term& term, size_t bits = 0);
    Dynamic getExprForValue(
        const llvm::Value& value,
        const std::string& name
    );
    // Valid/invalid pointers
    Pointer getInvalidPtr();
    Bool isInvalidPtrExpr(Pointer ptr);
    // Misc pointer stuff
    Bool getDistinct(const std::vector<Pointer>& exprs);

#include "Util/macros.h"
    auto if_(Bool cond) QUICK_RETURN(logic::if_(cond))
#include "Util/unmacros.h"

    template<class T, class U>
    T switch_(
        U val,
        const std::vector<std::pair<U, T>>& cases,
        T default_) {
        return logic::switch_(val, cases, default_);
    }

private:

    expr to_expr(Z3_ast ast);

    Dynamic getExprByTypeAndName(
            const llvm::ValueType type,
            const std::string& name,
            size_t bitsize = 0);

public:

    static void initialize(llvm::TargetData* TD);

private:

    z3::context& ctx;

    static unsigned int pointerSize;

};

} /* namespace borealis */

#endif /* Z3EXPRFACTORY_H_ */
