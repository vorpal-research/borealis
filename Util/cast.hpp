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
    typedef std::pair<T1, T2> datty;
    typedef std::unique_ptr<datty> datptr;
    datptr data;

    datty* operator ->() {
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
	typedef	typename pair_matcher<R1, R2> Retty;
    typedef typename Retty::datty datty;
    typedef typename Retty::datptr datptr;

    if (auto r1 = dyn_cast<T1>(q1)) {
        if (auto r2 = dyn_cast<T2>(q2)) {
            return Retty { datptr(new datty(r1, r2)) };
        }
    }

    return Retty { nullptr };
}

////////////////////////////////////////////////////////////////////////////////
//
// "dyn_cast for a tuple" kinda stuff
//
////////////////////////////////////////////////////////////////////////////////
/*******************************************************************************
template<class RettyTypeList>
struct tuple_matcher;

template<class ...Retty>
struct tuple_matcher<type_list<Retty...>> {
    typedef std::tuple<Retty...> datty;
    typedef std::unique_ptr<datty> datptr;
    datptr data;

    datty* operator ->() {
        return data.get();
    }

    const datty& operator*() {
        return *data.get();
    }

    operator void*() {
        return data.get();
    }

    template<size_t Idx>
    typename index_in_row<Idx, Retty...>::type
    get() {
        return std::get<Idx>(*data.get());
    }
};



namespace impl {
template<class T, class Q>
struct get_cast_retty;

template<class T1, class ...T, class Q1, class ...Q>
struct get_cast_retty<type_list<T1, T...>, type_list<Q1, Q...>> {
    typedef typename get_cast_retty<type_list<T...>, type_list<Q...>>::type rest;
    typedef typename llvm::cast_retty<T1, Q1>::ret_type self;
    typedef typename cons<self, rest>::type type;
};

template<class T1, class Q1>
struct get_cast_retty<type_list<T1>, type_list<Q1>> {
    typedef typename llvm::cast_retty<T1, Q1>::ret_type self;
    typedef type_list<self> type;
};
} // namespace impl



template<class T1, class ...T>
struct match_tuple {

    template<class Q1, class Q2, class ...Q>
    static
    tuple_matcher<
        typename impl::get_cast_retty<type_list<T1, T...>, type_list<Q1, Q2, Q...>>::type
    >
    doit(const Q1& q1, const Q2& q2, const Q&... qs) {
        using llvm::dyn_cast;

        typedef
        tuple_matcher<
            typename impl::get_cast_retty<type_list<T1, T...>, type_list<Q1, Q2, Q...>>::type
        >
        Retty;
        typedef typename Retty::datty datty;
        typedef typename Retty::datptr datptr;

        if (auto r1 = dyn_cast<T1>(q1)) {
            if (auto rs = match_tuple<T...>::doit(q2, qs...)) {

                auto res = std::tuple_cat(std::make_tuple(r1), *rs);

                return Retty { datptr(new datty(res)) };

            }
        }

        return Retty { nullptr };
    }

    template<class Q1>
    static
    tuple_matcher<
        typename impl::get_cast_retty<type_list<T1>, type_list<Q1>>::type
    >
    doit(const Q1& q1) {
        using llvm::dyn_cast;

        typedef
        tuple_matcher<
            typename impl::get_cast_retty<type_list<T1>, type_list<Q1>>::type
        >
        Retty;
        typedef typename Retty::datty datty;
        typedef typename Retty::datptr datptr;

        if (auto r1 = dyn_cast<T1>(q1)) {
            return Retty { datptr(new datty(r1)) };
        }

        return Retty { nullptr };
    }

};
*******************************************************************************/

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* CAST_HPP_ */
