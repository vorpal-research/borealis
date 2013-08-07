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
// Type combinators
//
////////////////////////////////////////////////////////////////////////////////

template<class F, class _>
struct type_K_comb {
    typedef F type;
};

template<class F, class S>
using type_K_comb_q = typename type_K_comb<F,S>::type;

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
    static_assert(sizeof...(Args) > 0, "tl_cdr with 1-element type list");
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
struct is_T_in<T, T, Tail...> : std::true_type {};

template<class T, class H, class ...Tail>
struct is_T_in<T, H, Tail...> : is_T_in<T, Tail...> {};

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
struct index_in_row<0U, Head, Tail...> {
    typedef Head type;
};

template<size_t I, class _, class ...Tail>
struct index_in_row<I, _, Tail...> {
    typedef typename index_in_row<I-1, Tail...>::type type;
};

template<size_t I, class ...List>
using index_in_row_q = typename index_in_row<I, List...>::type;

template<class ...List>
using head_of_row_q = typename index_in_row<0U, List...>::type;

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
struct make_indexer : public make_indexer_from<0U, sizeof...(Args)-1> {};

template<class ...Args>
struct make_cdr_indexer : public make_indexer_from<1U, sizeof...(Args)-1> {};

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

template<size_t I, class TypeList>
using index_in_type_list_q = typename index_in_type_list<I, TypeList>::type;

////////////////////////////////////////////////////////////////////////////////
//
// static_string
//
////////////////////////////////////////////////////////////////////////////////

template<char ...Args>
struct static_string {
    enum { length = sizeof...(Args) };
    typedef const char(&array_ref)[sizeof...(Args)+1];

    static array_ref c_str() {
        static const char output[]{ Args..., '\0' };
        return output;
    }

    operator array_ref() const {
        return c_str();
    }

    operator const char*() const {
        return c_str();
    }

    friend std::ostream& operator<<(std::ostream& ost, static_string<Args...>) {
        return ost << static_string::c_str();
    }
};

template<size_t Sz>
constexpr char at(char const(&s)[Sz], size_t i) {
    return i >= Sz ? '\0' : s[i];
}

template<class SS, char wha>
struct ss_append_char;

template<char ...Str>
struct ss_append_char<static_string<Str...>, '\0'> {
    typedef static_string<Str...> type;
};

template<char wha, char ...Str>
struct ss_append_char<static_string<Str...>, wha> {
    typedef static_string<Str..., wha> type;
};

template<class SS, char wha>
using ss_append_char_q = typename ss_append_char<SS, wha>::type;

template<class SS, char ...wha>
struct ss_append_chars;

template<class SS>
struct ss_append_chars<SS> {
    typedef SS type;
};

template<class SS, char Head, char ...Tail>
struct ss_append_chars<SS, Head, Tail...> {
    typedef typename ss_append_char<SS, Head>::type headed;
    typedef typename ss_append_chars<headed, Tail...>::type type;
};

template<class SS, char ...wha>
using ss_append_chars_q = typename ss_append_chars<SS, wha...>::type;

template<char ...wha>
using make_ss = typename ss_append_chars<static_string<>, wha...>::type;

////////////////////////////////////////////////////////////////////////////////
//
// a la type_traits
//
////////////////////////////////////////////////////////////////////////////////

template<class Ptr>
using make_const_ptr_q = typename std::add_pointer<
    typename std::add_const<
        typename std::remove_pointer<Ptr>::type
    >::type
>::type;

template<class Ptr>
using unmake_const_ptr_q = typename std::add_pointer<
    typename std::remove_const<
        typename std::remove_pointer<Ptr>::type
    >::type
>::type;

} // namespace util
} // namespace borealis

#endif /* META_HPP_ */
