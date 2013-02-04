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

template<class T, class ...List>
struct is_T_in;

template<class T, class ...Tail>
struct is_T_in<T,T,Tail...> : std::true_type {};

template<class T, class H, class ...Tail>
struct is_T_in<T,H,Tail...> : is_T_in<T, Tail...> {};

template<class T>
struct is_T_in<T> : std::false_type {};

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

template<size_t ...N>
struct indexer {};

template<size_t N, class Indexer>
struct glue_indexer;

template<size_t N, size_t ...Row>
struct glue_indexer<N, indexer<Row...>> {
    typedef indexer<Row..., N> type;
};

template<class ...Args>
struct make_indexer;

template<class Head>
struct make_indexer<Head> {
    typedef indexer<0> type;
};

template<class Head0, class Head1, class ...Tail>
struct make_indexer<Head0, Head1, Tail...> {
    typedef typename make_indexer<Head1, Tail...>::type prev;
    typedef typename glue_indexer<sizeof...(Tail)+1, prev>::type type;
};

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
