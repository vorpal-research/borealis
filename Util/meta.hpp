/*
 * meta.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef META_HPP_
#define META_HPP_

#include <type_traits>

namespace borealis {
namespace util {

// DO NOT DEFINE! decltype resolving only
template<class T> T some();

////////////////////////////////////////////////////////////////////////////////
//
// Types
//
////////////////////////////////////////////////////////////////////////////////

template<size_t ...N>
struct indexer {};

template<class ...List>
struct type_list {};

////////////////////////////////////////////////////////////////////////////////
//
// type_list stuff
//
////////////////////////////////////////////////////////////////////////////////

template<class List>
struct isTypeList : std::false_type {};

template<class ...Args>
struct isTypeList< type_list<Args...> > : std::true_type {};

// car
template<class List>
struct tl_car;

template<class H, class ...Args>
struct tl_car< type_list<H, Args...> > {
    typedef H type;
};

template<class TList>
using tl_car_q = typename tl_car<TList>::type;

// cdr
template<class List>
struct tl_cdr;

template<class H, class ...Args>
struct tl_cdr< type_list<H, Args...> > {
    // static_assert(sizeof...(Args) > 0, "tl_cdr with 1-element type list");
    typedef type_list<Args...> type;
};

template<class TList>
using tl_cdr_q = typename tl_cdr<TList>::type;

// type_list -> tuple
template<class List>
struct tl_to_tuple;

template<class ...Args>
struct tl_to_tuple< type_list<Args...> > {
    typedef std::tuple<Args...> type;
};

template<class TList>
using tl_to_tuple_q = typename tl_to_tuple<TList>::type;

// tuple -> type_list
template<class Tuple>
struct tl_from_tuple;

template<class ...Args>
struct tl_from_tuple< std::tuple<Args...> > {
    typedef type_list<Args...> type;
};

template<class Tuple>
using tl_from_tuple_q = typename tl_from_tuple<Tuple>::type;

////////////////////////////////////////////////////////////////////////////////
//
// cdr / cons / append
//
////////////////////////////////////////////////////////////////////////////////

template<class List>
struct cdr;

template<size_t Head, size_t ...Tail>
struct cdr< indexer<Head, Tail...> > {
    static_assert(sizeof...(Tail) > 0, "cdr with 1-element indexer");
    typedef indexer<Tail...> type;
};


template<class E, class List>
struct cons;

template<class E, class ...List>
struct cons<E, type_list<List...>> {
    typedef type_list<E, List...> type;
};


template<class E, class List>
struct append;

template<class E, class ...List>
struct append<E, type_list<List...>> {
    typedef type_list<List..., E> type;
};

////////////////////////////////////////////////////////////////////////////////
//
// get_index_of_T_in
//
////////////////////////////////////////////////////////////////////////////////

template<class T, class ...List>
struct get_index_of_T_in;

// Head == T
template<class T, class ...Tail>
struct get_index_of_T_in<T, T, Tail...> {
    enum { value = 0 };
};

// Head != T
template<class T, class Head, class ...Tail>
struct get_index_of_T_in<T, Head, Tail...> {
    enum { value = get_index_of_T_in<T, Tail...>::value + 1 };
};

////////////////////////////////////////////////////////////////////////////////
//
// is_T_in
//
////////////////////////////////////////////////////////////////////////////////

template<class T, class ...List>
struct is_T_in;

template<class T, class ...Tail>
struct is_T_in<T,T,Tail...> : std::true_type {};

template<class T, class H, class ...Tail>
struct is_T_in<T,H,Tail...> : is_T_in<T, Tail...> {};

template<class T>
struct is_T_in<T> : std::false_type {};

////////////////////////////////////////////////////////////////////////////////
//
// index_in_row
//
////////////////////////////////////////////////////////////////////////////////

template<size_t I, class ...List>
struct index_in_row;

template<class Head, class ...Tail>
struct index_in_row<0, Head, Tail...> {
    typedef Head type;
};

template<size_t I, class Ignore, class ...Tail>
struct index_in_row<I, Ignore, Tail...> {
    typedef typename index_in_row<I-1, Tail...>::type type;
};

template<size_t I, class ...List>
using index_in_row_q = typename index_in_row<I, List...>::type;

////////////////////////////////////////////////////////////////////////////////
//
// make_indexer_from
//
////////////////////////////////////////////////////////////////////////////////

template<size_t N, class Indexer>
struct glue_indexer;

template<size_t N, size_t ...Row>
struct glue_indexer<N, indexer<Row...>> {
    typedef indexer<Row..., N> type;
};


template<size_t From, size_t To>
struct make_indexer_from {
    static_assert(To > From, "Cannot make decreasing indexer");
    typedef typename make_indexer_from<From, To-1>::type prev;
    typedef typename glue_indexer<To, prev>::type type;
};

template<size_t From>
struct make_indexer_from<From, From> {
    typedef indexer<From> type;
};


template<class ...Args>
struct make_indexer : public make_indexer_from<0, sizeof...(Args)-1> {};

template<class ...Args>
struct make_cdr_indexer : public make_indexer_from<1, sizeof...(Args)-1> {};


////////////////////////////////////////////////////////////////////////////////
//
// index_in_type_list
//
////////////////////////////////////////////////////////////////////////////////

template<size_t I, class TypeList>
struct index_in_type_list;

template<size_t I, class ...Args>
struct index_in_type_list<I, type_list<Args...>> {
    typedef typename index_in_row<I, Args...>::type type;
};

} // namespace util
} // namespace borealis

#endif /* META_HPP_ */
