/*
 * cast.hpp
 *
 *  Created on: Jan 24, 2013
 *      Author: belyaev
 */

#ifndef CAST_HPP_
#define CAST_HPP_

#include <llvm/Support/Casting.h>

#include <utility>

#include "Util/meta.hpp"
#include "Util/option.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

template<class Basic>
class llvm_rtti_visitor {
    const Basic* b;

public:
    llvm_rtti_visitor(const Basic* b) : b(b) {};
    llvm_rtti_visitor(std::shared_ptr<Basic> b) : b(b.get()) {};
    llvm_rtti_visitor(const Basic& b) : b(&b) {};
    llvm_rtti_visitor(const llvm_rtti_visitor& b) = default;

    template<class Derived, class Callable>
    auto on(Callable c) -> option<decltype(c(some<Derived>()))> {
        if(auto der = llvm::dyn_cast_or_null<Derived>(b)) {
            return just(c(*der));
        }
        return nothing();
    }

    template<class Derived, class Callable>
    llvm_rtti_visitor& oncase(Callable c) {
        if(auto der = llvm::dyn_cast_or_null<Derived>(b)) {
            c(*der);
        }
        return *this;
    }
};

template<class Basic>
llvm_rtti_visitor<Basic> visit(const Basic& b) {
    return b;
}

template<class Basic>
llvm_rtti_visitor<Basic> visit(const Basic* b) {
    return b;
}

////////////////////////////////////////////////////////////////////////////////
//
// "dyn_cast for a pair" kinda stuff
//
////////////////////////////////////////////////////////////////////////////////

template<class T1, class T2>
struct pair_matcher {
    std::unique_ptr< std::pair<T1, T2> > data;

    std::pair<T1, T2>* operator ->() {
        return data.get();
    }

    operator void*() {
        return data.get();
    }
};

template<class T1, class T2, class Q1, class Q2>
pair_matcher<
    typename llvm::cast_retty<T1, Q1>::ret_type,
    typename llvm::cast_retty<T2, Q2>::ret_type
> match_pair(const Q1& q1, const Q2& q2) {
    using llvm::dyn_cast;

    typedef typename llvm::cast_retty<T1, Q1>::ret_type R1;
    typedef typename llvm::cast_retty<T2, Q2>::ret_type R2;

    if (auto t1 = dyn_cast<T1>(q1)) {
        if (auto t2 = dyn_cast<T2>(q2)) {
            return pair_matcher<R1, R2>{ std::unique_ptr< std::pair<R1, R2> >(new std::pair<R1, R2>(t1, t2)) };
        }
    }

    return pair_matcher<R1, R2>{ nullptr };
}

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* CAST_HPP_ */
