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

#include "Solver/Logic.hpp"
#include "Type/TypeFactory.h"
#include "Util/util.h"

namespace borealis {

class Term;

class Z3ExprFactory {

public:

    typedef z3::func_decl function;
    typedef z3::func_decl array;
    typedef z3::expr expr;
    typedef z3::sort sort;
    typedef std::vector<expr> expr_vector;

    // a hack: CopyAssignable reference to non-CopyAssignable object
    // (z3::expr is CopyConstructible, but not CopyAssignable, so no
    // accumulator-like shit is possible with it)
    // typedef borealis::util::copyref<expr> exprRef;

    typedef borealis::logic::Bool Bool;
    // logic type to represent pointers
    typedef borealis::logic::BitVector<32> Pointer;
    // logic type to represent integers
    typedef borealis::logic::BitVector<Pointer::bitsize> Integer;
    // logic type to represent reals
    typedef borealis::logic::BitVector<Pointer::bitsize> Real;
    // logic type to represent memory units
    typedef borealis::logic::BitVector<Pointer::bitsize> Byte;
    // memory array
    template<class Elem, class Index> using ArrayImpl = logic::TheoryArray<Elem, Index>;
    typedef borealis::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl> MemArray;
    // dynamic bit vector
    typedef borealis::logic::DynBitVectorExpr DynBV;
    // dynamic logic type
    typedef borealis::logic::SomeExpr Dynamic;

    static size_t sizeForType(Type::Ptr type) {
        using llvm::isa;
        return isa<borealis::Integer>(type) ? Integer::bitsize :
               isa<borealis::Pointer>(type) ? Pointer::bitsize :
               isa<borealis::Float>(type) ? Real::bitsize :
               util::sayonara<size_t>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                       "Cannot acquire bitsize for type " + util::toString(type));
    }

    Z3ExprFactory();

    z3::context& unwrap() {
        return *ctx;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Pointers
    Pointer getPtrVar(const std::string& name, bool fresh = false);
    Pointer getPtrConst(int ptr);
    Pointer getNullPtr();
    // Bools
    Bool getBoolVar(const std::string& name, bool fresh = false);
    Bool getBoolConst(bool b);
    Bool getTrue();
    Bool getFalse();
    // Integers
    Integer getIntVar(const std::string& name, bool fresh = false);
    Integer getIntConst(int v);
    // Reals
    Real getRealVar(const std::string& name, bool fresh = false);
    Real getRealConst(int v);
    Real getRealConst(double v);
    // Memory
    MemArray getNoMemoryArray();

    // Generic functions
    Dynamic getVarByTypeAndName(
            Type::Ptr type,
            const std::string& name,
            bool fresh = false);

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

public:

    static void initialize(llvm::TargetData* TD);

private:

    std::unique_ptr<z3::context> ctx;

    static unsigned int pointerSize;

};

} /* namespace borealis */

#endif /* Z3EXPRFACTORY_H_ */
