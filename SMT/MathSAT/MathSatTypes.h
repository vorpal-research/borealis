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
	typedef mathsat_::ExprFactory ExprFactory;
	typedef mathsat_::ExecutionContext ExecutionContext;
	typedef mathsat_::Solver Solver;

    // logic type to represent boolean expressions
    typedef mathsat_::logic::Bool Bool;
    // logic type to represent pointers
    typedef mathsat_::logic::BitVector<32> Pointer;
    // logic type to represent memory units
    typedef mathsat_::logic::BitVector<Pointer::bitsize> Byte;
    // logic type to represent integers
    typedef mathsat_::logic::BitVector<Pointer::bitsize> Integer;
    // logic type to represent reals
    typedef mathsat_::logic::BitVector<Pointer::bitsize> Real;
    // dynamic bit vector
    typedef mathsat_::logic::DynBitVectorExpr DynBV;
    // dynamic logic type
    typedef mathsat_::logic::SomeExpr Dynamic;

    // memory array
    template<class Elem, class Index> using ArrayImpl = mathsat_::logic::InlinedFuncArray<Elem, Index>;
    typedef mathsat_::logic::ScatterArray<Pointer, Byte::bitsize, ArrayImpl> MemArray;

};

} // namespace borealis

#endif /* BOREALIS_MATHSATTYPES_H_ */
