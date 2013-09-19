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

    typedef z3_::ExprFactory ExprFactory;
    typedef z3_::ExecutionContext ExecutionContext;
    typedef z3_::Solver Solver;

    // logic type to represent boolean expressions
    typedef z3_::logic::Bool Bool;
    // logic type to represent pointers
    typedef z3_::logic::BitVector<32> Pointer;
    // logic type to represent memory units
    typedef z3_::logic::BitVector<Pointer::bitsize> Byte;
    // logic type to represent integers
    typedef z3_::logic::BitVector<Pointer::bitsize> Integer;
    // logic type to represent reals
    typedef z3_::logic::BitVector<Pointer::bitsize> Real;
    // dynamic bit vector
    typedef z3_::logic::DynBitVectorExpr DynBV;
    // dynamic logic type
    typedef z3_::logic::SomeExpr Dynamic;

    // memory array
//    template<class Elem, class Index> using ArrayImpl = z3_::logic::TheoryArray<Elem, Index>;
    template<class Elem, class Index> using ArrayImpl = z3_::logic::FuncArray<Elem, Index>;
    typedef z3_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl> MemArray;

};

} // namespace borealis

#endif /* BOREALIS_Z3_H_ */
