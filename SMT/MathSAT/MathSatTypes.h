/*
 * MathSatTypes.h
 *
 *  Created on: Aug 2, 2013
 *      Author: sam
 */

#ifndef BOREALIS_MATHSATTYPES_H_
#define BOREALIS_MATHSATTYPES_H_

#include "SMT/SMT.hpp"
#include "SMT/MathSAT/Logic.hpp"

namespace borealis {

namespace mathsat_ {
class ExprFactory;
class ExecutionContext;
class Solver;
}

struct MathSAT {
    using ExprFactory = mathsat_::ExprFactory;
    using ExecutionContext = mathsat_::ExecutionContext;
    using Solver = mathsat_::Solver;

    // logic type to represent boolean expressions
    using Bool = mathsat_::logic::Bool;
    // logic type to represent pointers
    using Pointer = mathsat_::logic::BitVector<64>;
    // logic type to represent memory units
    using Byte = mathsat_::logic::BitVector<64>;
    // logic type to represent integers
    using Integer = mathsat_::logic::DynBitVectorExpr;
    // logic type to represent reals
    using Real = mathsat_::logic::BitVector<64>;
    // bit vector
    template<size_t N>
    using BV = mathsat_::logic::BitVector<N>;
    // dynamic bit vector
    using DynBV = mathsat_::logic::DynBitVectorExpr;
    // unsigned comparable type
    using UComparable = mathsat_::logic::UComparableExpr;
    // dynamic logic type
    using Dynamic = mathsat_::logic::SomeExpr;

    template<class Elem, class Index> using ArrayImpl = mathsat_::logic::InlinedFuncArray<Elem, Index>;

    // memory array
    using MemArray = mathsat_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl>;

};

} // namespace borealis

#endif /* BOREALIS_MATHSATTYPES_H_ */
