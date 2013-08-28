/*
 * type_traits.hpp
 *
 *  Created on: Aug 22, 2013
 *      Author: belyaev
 */

#ifndef TYPE_TRAITS_HPP_
#define TYPE_TRAITS_HPP_

#include <type_traits>

namespace borealis {
namespace util {

template<class T>
using decay_t = typename std::decay<T>::type;

template<class T>
using remove_reference_t = typename std::remove_reference<T>::type;

template<class T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template<class T>
using underlying_type_t = typename std::underlying_type<T>::type;

} // namespace util
} // namespace borealis



#endif /* TYPE_TRAITS_HPP_ */
