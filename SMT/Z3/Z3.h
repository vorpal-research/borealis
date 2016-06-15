/*
 * Z3.h
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef BOREALIS_Z3_H_
#define BOREALIS_Z3_H_

#include "SMT/SMT.hpp"
#include "SMT/Z3/Logic.hpp"

namespace borealis {

namespace z3_ {
class ExprFactory;
class ExecutionContext;
class Solver;
}

struct Z3 {
    using Engine = Z3Engine;

    using ExprFactory = z3_::ExprFactory;
    using ExecutionContext = z3_::ExecutionContext;
    using Solver = z3_::Solver;

    // logic type to represent boolean expressions
    using Bool = z3_::logic::Bool;
    // logic type to represent pointers
    using Pointer = z3_::logic::BitVector<64>;
    // logic type to represent memory units
    using Byte = z3_::logic::BitVector<64>;
    // logic type to represent integers
    using Integer = z3_::logic::AnyBitVector;
    // logic type to represent reals
    using Real = z3_::logic::BitVector<64>;
    // bit vector
    template<size_t N>
    using BV = z3_::logic::BitVector<N>;
    // dynamic bit vector
    using DynBV = z3_::logic::AnyBitVector;
    // unsigned comparable type
    using UComparable = z3_::logic::AnyBitVector;
    // dynamic logic type
    using Dynamic = z3_::logic::ValueExpr;

    using add_no_overflow = z3_::logic::add_no_overflow;

#if defined USE_FUNC_ARRAY
    template<class Elem, class Index> using ArrayImpl = z3_::logic::FuncArray<Elem, Index>;
#elif defined USE_INLINED_FUNC_ARRAY
    template<class Elem, class Index> using ArrayImpl = z3_::logic::InlinedFuncArray<Elem, Index>;
#else
    template<class Elem, class Index> using ArrayImpl = z3_::logic::TheoryArray<Elem, Index>;
#endif

    // memory array
    using MemArray = z3_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl>;
    
};

} // namespace borealis

namespace z3 {

borealis::logging::logstream& operator<<(borealis::logging::logstream& os, const z3::expr_vector& z3ev);
borealis::logging::logstream& operator<<(borealis::logging::logstream& os, const z3::func_entry& z3fe);
borealis::logging::logstream& operator<<(borealis::logging::logstream& os, const z3::func_interp& z3fi);

} // namespace z3

#endif /* BOREALIS_Z3_H_ */
