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
        if (auto* der = llvm::dyn_cast_or_null<Derived>(b)) {
            return just(c(*der));
        }
        return nothing();
    }

    template<class Derived, class Callable>
    llvm_rtti_visitor& oncase(Callable c) {
        if (auto* der = llvm::dyn_cast_or_null<Derived>(b)) {
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

    datty* operator->() {
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
	typedef	pair_matcher<R1, R2> Retty;
    typedef typename Retty::datty datty;
    typedef typename Retty::datptr datptr;

    if (auto r1 = dyn_cast<T1>(q1)) {
        if (auto r2 = dyn_cast<T2>(q2)) {
            return Retty{ datptr{ new datty{r1, r2} } };
        }
    }

    return Retty{ nullptr };
}

////////////////////////////////////////////////////////////////////////////////
//
// "dyn_cast for a tuple" kinda stuff
//
////////////////////////////////////////////////////////////////////////////////

template<class RettyTypeList>
struct tuple_matcher;

template<class ...Retty>
struct tuple_matcher< type_list<Retty...> > {

    typedef type_list<Retty...> TheList;
    typedef tl_car_q<TheList> data_t;
    typedef std::unique_ptr<tuple_matcher<tl_cdr_q<TheList>>> tail_t;

    typedef tuple_matcher< TheList > self;
    typedef std::unique_ptr<self> Ptr;

    template<size_t Idx>
    using RetNum = typename index_in_row<Idx, Retty...>::type;

    data_t head;
    tail_t tail;

    template<size_t Idx>
    RetNum<Idx> get() {
        static_assert(Idx >= 0,               "tuple_matcher::get with negative index");
        static_assert(Idx < sizeof...(Retty), "tuple_matcher::get with index out-of-bounds");
        return get_<Idx>(std::integral_constant<bool, Idx == 0>());
    }

    template<size_t Idx>
    RetNum<0U> get_(std::true_type) {
        return head;
    }

    template<size_t Idx>
    RetNum<Idx> get_(std::false_type) {
        return tail->template get<Idx-1>();
    }
};

template<class R>
struct tuple_matcher< type_list<R> > {
    typedef R data_t;

    typedef tuple_matcher< type_list<R> > self;
    typedef std::unique_ptr<self> Ptr;

    data_t head;

    template<size_t Idx>
    R get() {
        static_assert(Idx == 0, "tuple_matcher::get with incorrect index");
        return head;
    }
};



namespace impl {
template<class TL, class QL>
struct get_cast_retty {
    typedef typename get_cast_retty<tl_cdr_q<TL>, tl_cdr_q<QL>>::type rest;
    typedef typename llvm::cast_retty<tl_car_q<TL>, tl_car_q<QL>>::ret_type self;
    typedef typename cons<self, rest>::type type;
};

template<class T, class Q>
struct get_cast_retty<type_list<T>, type_list<Q>> {
    typedef typename llvm::cast_retty<T, Q>::ret_type self;
    typedef type_list<self> type;
};
} // namespace impl



template<class T1, class ...T>
struct match_tuple {

    template<class U1, class ...U>
    using Rettype = tuple_matcher<
        typename impl::get_cast_retty<type_list<T1, T...>, type_list<U1, U...>>::type
    >;

    template<class U1, class ...U>
    static typename Rettype<U1, U...>::Ptr doit(const U1& u, const U&... us) {
        using llvm::dyn_cast;

        typedef Rettype<U1, U...> Retty;
        typedef typename Retty::Ptr RettyPtr;

        if (auto r1 = dyn_cast<T1>(u)) {
            if (auto rs = match_tuple<T...>::doit(us...)) {
                return RettyPtr{
                    new Retty{
                        r1,
                        std::move(rs)
                    }
                };
            }
        }

        return RettyPtr{ nullptr };
    }

    template<class U>
    static typename Rettype<U>::Ptr doit(const U& u) {
        using llvm::dyn_cast;

        typedef Rettype<U> Retty;
        typedef typename Retty::Ptr RettyPtr;

        if (auto r1 = dyn_cast<T1>(u)) {
            return RettyPtr{
                new Retty{
                    r1
                }
            };
        }

        return RettyPtr{ nullptr };
    }

};

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* CAST_HPP_ */
