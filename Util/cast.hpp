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

#include "Util/util.hpp"
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
	typedef	pair_matcher<R1, R2> Retty;
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
/*******************************************************************************/
template<class RettyTypeList>
struct tuple_matcher;

template<class ...Retty>
struct tuple_matcher<type_list<Retty...>> {
    typedef type_list<Retty...> theList;
    typedef tl_car_q<theList> data_t;
    typedef tuple_matcher<tl_cdr_q<theList>> tail_t;

    template<size_t Ix>
    using RetNum = typename index_in_row<Ix, Retty...>::type;

    data_t* head;
    tail_t* tail;

    operator void*() {
        return reinterpret_cast<void*>(head);
    }

    template<size_t Idx>
    RetNum<Idx> get() {
        return get_<Idx>(std::integral_constant<bool, Idx == 0>());
    }

    template<size_t>
    RetNum<0U> get_(std::true_type) {
        return *head;
    }

    template<size_t Idx>
    RetNum<Idx> get_(std::false_type) {
        return tail->template get<Idx-1>();
    }
};


namespace impl {
template<class TL, class QL>
struct get_cast_retty {

    typedef typename get_cast_retty<tl_cdr_q<TL>, tl_cdr_q<QL>>::type rest;
    typedef typename llvm::cast_retty<tl_car_q<TL>, tl_car_q<QL>>::ret_type self;
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

    typedef type_list<T1, T...> tlist;

    template<class Tuple>
    using rettype = tuple_matcher<
       typename impl::get_cast_retty<tlist, tl_from_tuple_q<Tuple>>::type
    >;

    template<class U>
    static rettype<U> doit_(const U& v, std::true_type) {
        using llvm::dyn_cast;

        typedef rettype<U> Retty;

        if (auto r1 = dyn_cast<T1>(std::get<0>(v))) {
            typedef match_tuple<T...> mtuple;
            if (auto rs = mtuple::doit(cdr_tuple(v))) {
                return Retty {
                    borealis::util::heap_copy(&r1),
                    borealis::util::heap_copy(&rs)
                };

            }
        }

        return rettype<U> { nullptr, nullptr };
    }

    template<class U>
    static rettype<U> doit_(const U& v, std::false_type) {
        using llvm::dyn_cast;

        typedef rettype<U> Retty;

        if (auto r1 = dyn_cast<T1>(std::get<0>(v))) {
            return Retty {
                borealis::util::heap_copy(&r1),
                nullptr
            };
        }

        return rettype<U> { nullptr, nullptr };
    }

    template<class U>
    static rettype<U> doit(const U& v) {
        return doit_(v, std::integral_constant<bool, (std::tuple_size<U>::value > 1)>());
    }

};
/*******************************************************************************/

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* CAST_HPP_ */
