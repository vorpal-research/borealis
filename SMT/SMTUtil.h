/*
 * SMTUtil.h
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef BOREALIS_SMTUTIL_H_
#define BOREALIS_SMTUTIL_H_

namespace borealis {

#define BRING_FROM_IMPL(CLASS) \
    template<class Impl> \
    using CLASS = typename Impl::CLASS;

BRING_FROM_IMPL(ExprFactory)
BRING_FROM_IMPL(ExecutionContext)
BRING_FROM_IMPL(Solver)

BRING_FROM_IMPL(Bool)
BRING_FROM_IMPL(Byte)
BRING_FROM_IMPL(Pointer)
BRING_FROM_IMPL(Integer)
BRING_FROM_IMPL(Real)
BRING_FROM_IMPL(DynBV)
BRING_FROM_IMPL(Dynamic)

BRING_FROM_IMPL(MemArray)

#undef BRING_FROM_IMPL



#define USING_SMT_LOGIC(IMPL) \
    typedef typename IMPL::Bool Bool; \
    typedef typename IMPL::Byte Byte; \
    typedef typename IMPL::Pointer Pointer; \
    typedef typename IMPL::Integer Integer; \
    typedef typename IMPL::Real Real; \
    typedef typename IMPL::DynBV DynBV; \
    typedef typename IMPL::UComparable UComparable; \
    typedef typename IMPL::Dynamic Dynamic; \
    typedef typename IMPL::MemArray MemArray;

#define USING_SMT_IMPL(IMPL) \
    USING_SMT_LOGIC(IMPL) \
    typedef typename IMPL::ExprFactory ExprFactory; \
    typedef typename IMPL::ExecutionContext ExecutionContext; \
    typedef typename IMPL::Solver Solver;



template<class Impl, class SubClass>
struct SMTImpl;

template<class Impl>
struct SMT;

} // namespace borealis

#endif /* BOREALIS_SMTUTIL_H_ */
