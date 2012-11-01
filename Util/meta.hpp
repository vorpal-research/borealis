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

template<class T, class ...List>
struct get_index_of_T_in;

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

}
}


#endif /* META_HPP_ */
