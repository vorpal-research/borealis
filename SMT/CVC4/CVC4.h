/*
 * Z3.h
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef CVC4_H_
#define CVC4_H_

#include "SMT/SMT.hpp"
#include "SMT/CVC4/Logic.hpp"

namespace borealis {

namespace cvc4_ {
class ExprFactory;
class ExecutionContext;
class Solver;
}

struct CVC4 {
    using Engine = CVC4Engine;

    using ExprFactory = cvc4_::ExprFactory;
    using ExecutionContext = cvc4_::ExecutionContext;
    using Solver = cvc4_::Solver;

    // logic type to represent boolean expressions
    using Bool = cvc4_::logic::Bool;
    // logic type to represent pointers
    using Pointer = cvc4_::logic::BitVector<64>;
    // logic type to represent memory units
    using Byte = cvc4_::logic::BitVector<64>;
    // logic type to represent integers
    using Integer = cvc4_::logic::AnyBitVector;
    // logic type to represent reals
    using Real = cvc4_::logic::BitVector<64>;
    // bit vector
    template<size_t N>
    using BV = cvc4_::logic::BitVector<N>;
    // dynamic bit vector
    using DynBV = cvc4_::logic::AnyBitVector;
    // unsigned comparable type
    using UComparable = cvc4_::logic::AnyBitVector;
    // dynamic logic type
    using Dynamic = cvc4_::logic::ValueExpr;

    using add_no_overflow = cvc4_::logic::add_no_overflow;

#if defined USE_FUNC_ARRAY
    template<class Elem, class Index> using ArrayImpl = cvc4_::logic::FuncArray<Elem, Index>;
#elif defined USE_INLINED_FUNC_ARRAY
    template<class Elem, class Index> using ArrayImpl = cvc4_::logic::InlinedFuncArray<Elem, Index>;
#else
    template<class Elem, class Index> using ArrayImpl = cvc4_::logic::TheoryArray<Elem, Index>;
#endif

    // memory array
    using MemArray = cvc4_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl>;
    
};

} // namespace borealis

#endif /* BOREALIS_CVC4_H_ */
