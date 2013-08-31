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
using add_const_reference_t = typename std::add_lvalue_reference<
    typename std::add_const<T>::type
>::type;

template<class T>
using underlying_type_t = typename std::underlying_type<T>::type;

template<class T>
using remove_cv_t = typename std::remove_cv<T>::type;

template<class T>
using remove_const_reference_t = remove_cv_t<remove_reference_t<T>>;

// FIXME: Should be applicable only to pointers
template<class T>
using make_const_ptr_t = typename std::add_pointer<
    typename std::add_const<
        typename std::remove_pointer<T>::type
    >::type
>::type;

// FIXME: Should be applicable only to pointers
template<class T>
using unmake_const_ptr_t = typename std::add_pointer<
    typename std::remove_const<
        typename std::remove_pointer<T>::type
    >::type
>::type;

} // namespace util
} // namespace borealis

#endif /* TYPE_TRAITS_HPP_ */
