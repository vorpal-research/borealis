/*
 * SMTUtil.h
 *
 *  Created on: Jul 18, 2013
 *      Author: ice-phoenix
 */

#ifndef BOREALIS_SMTUTIL_H_
#define BOREALIS_SMTUTIL_H_

#include <type_traits>
#include <unordered_map>

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
    using Bool = typename IMPL::Bool; \
    using Byte = typename IMPL::Byte; \
    using Pointer = typename IMPL::Pointer; \
    using Integer = typename IMPL::Integer; \
    using Real = typename IMPL::Real; \
    using DynBV = typename IMPL::DynBV; \
    using UComparable = typename IMPL::UComparable; \
    using Dynamic = typename IMPL::Dynamic; \
    using MemArray = typename IMPL::MemArray;

#define USING_SMT_IMPL(IMPL) \
    USING_SMT_LOGIC(IMPL) \
    using ExprFactory = typename IMPL::ExprFactory; \
    using ExecutionContext = typename IMPL::ExecutionContext; \
    using Solver = typename IMPL::Solver;



template<class Impl, class SubClass>
struct SMTImpl;

#define AUTO_CACHE_IMPL(PNAME, CTX, RESOLVE) \
    static std::unordered_map< std::decay_t< decltype(PNAME) >, Dynamic > cache; \
    static decltype(CTX) lastContext; \
    if(CTX != lastContext) cache.clear(); \
    lastContext = CTX; \
    if(cache.count(PNAME)) return cache[PNAME]; \
    Dynamic RESULT = RESOLVE; \
    return cache.emplace(PNAME, RESULT).second->second; \

template<class Impl>
struct SMT;

} // namespace borealis

#endif /* BOREALIS_SMTUTIL_H_ */
