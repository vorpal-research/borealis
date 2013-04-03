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

////////////////////////////////////////////////////////////////////////////////
//
// Indexers
//
////////////////////////////////////////////////////////////////////////////////

template<size_t ...N>
struct indexer {};

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
// cdr
//
////////////////////////////////////////////////////////////////////////////////

template<class What>
struct cdr;

template<size_t Head, size_t ...Tail>
struct cdr< indexer<Head, Tail...> > {
    static_assert(sizeof...(Tail) > 0, "cdr with 1-element indexer");
    typedef indexer<Tail...> type;
};

////////////////////////////////////////////////////////////////////////////////
//
// index_in_type_list
//
////////////////////////////////////////////////////////////////////////////////

template<class ...List>
struct type_list {};

template<size_t I, class Tl>
struct index_in_type_list;

template<size_t I, class ...Args>
struct index_in_type_list<I, type_list<Args...>> {
    typedef typename index_in_row<I, Args...>::type type;
};

} // namespace util
} // namespace borealis

#endif /* META_HPP_ */
