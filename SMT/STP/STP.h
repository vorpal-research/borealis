/*
 * Z3.h
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef STP_H_
#define STP_H_

#include "SMT/SMT.hpp"
#include "SMT/STP/Logic.hpp"

namespace borealis {

namespace stp_ {
class ExprFactory;
class ExecutionContext;
class Solver;
}

struct STP {
    using Engine = STPEngine;

    using ExprFactory = stp_::ExprFactory;
    using ExecutionContext = stp_::ExecutionContext;
    using Solver = stp_::Solver;

    // logic type to represent boolean expressions
    using Bool = stp_::logic::Bool;
    // logic type to represent pointers
    using Pointer = stp_::logic::BitVector<64>;
    // logic type to represent memory units
    using Byte = stp_::logic::BitVector<64>;
    // logic type to represent integers
    using Integer = stp_::logic::AnyBitVector;
    // logic type to represent reals
    using Real = stp_::logic::BitVector<64>;
    // bit vector
    template<size_t N>
    using BV = stp_::logic::BitVector<N>;
    // dynamic bit vector
    using DynBV = stp_::logic::AnyBitVector;
    // unsigned comparable type
    using UComparable = stp_::logic::AnyBitVector;
    // dynamic logic type
    using Dynamic = stp_::logic::ValueExpr;

    using add_no_overflow = stp_::logic::add_no_overflow;

    template<class Elem, class Index> using ArrayImpl = stp_::logic::TheoryArray<Elem, Index>;

    // memory array
    using MemArray = stp_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl>;
    
};

} // namespace borealis

#endif /* STP_H_ */
