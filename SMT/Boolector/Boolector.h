/*
 * Z3.h
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef BOOLECTOR_H_
#define BOOLECTOR_H_

#include "SMT/SMT.hpp"
#include "SMT/Boolector/Logic.hpp"

namespace borealis {

namespace boolector_ {
class ExprFactory;
class ExecutionContext;
class Solver;
}

struct Boolector {
    using Engine = BoolectorEngine;

    using ExprFactory = boolector_::ExprFactory;
    using ExecutionContext = boolector_::ExecutionContext;
    using Solver = boolector_::Solver;

    // logic type to represent boolean expressions
    using Bool = boolector_::logic::Bool;
    // logic type to represent pointers
    using Pointer = boolector_::logic::BitVector<64>;
    // logic type to represent memory units
    using Byte = boolector_::logic::BitVector<64>;
    // logic type to represent integers
    using Integer = boolector_::logic::AnyBitVector;
    // logic type to represent reals
    using Real = boolector_::logic::BitVector<64>;
    // bit vector
    template<size_t N>
    using BV = boolector_::logic::BitVector<N>;
    // dynamic bit vector
    using DynBV = boolector_::logic::AnyBitVector;
    // unsigned comparable type
    using UComparable = boolector_::logic::AnyBitVector;
    // dynamic logic type
    using Dynamic = boolector_::logic::ValueExpr;

    using add_no_overflow = boolector_::logic::add_no_overflow;

    template<class Elem, class Index> using ArrayImpl = boolector_::logic::TheoryArray<Elem, Index>;

    // memory array
    using MemArray = boolector_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl>;
    
};

} // namespace borealis

#endif /* BOREALIS_BOOLECTOR_H_ */
