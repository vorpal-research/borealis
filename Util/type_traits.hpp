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
using remove_pointer_t = typename std::remove_pointer<T>::type;

template<class T>
using remove_cvptr_t = remove_cv_t< remove_pointer_t < remove_cv_t<T> > >;

template<class T>
using remove_const_reference_t = remove_cv_t<remove_reference_t<T>>;

template<class T> struct make_const_ptr;
template<class T> struct make_const_ptr<T*>       { using type = const T*; };
template<class T> struct make_const_ptr<const T*> { using type = const T*; };

template<class T>
using make_const_ptr_t = typename make_const_ptr<T>::type;

template<class T> struct unmake_const_ptr;
template<class T> struct unmake_const_ptr<T*>       { using type = T*; };
template<class T> struct unmake_const_ptr<const T*> { using type = T*; };

template<class T>
using unmake_const_ptr_t = typename unmake_const_ptr<T>::type;

template<class U, class T> struct pointers_to_same {
    enum{ value = std::is_pointer<U>::value &&
                  std::is_pointer<T>::value &&
                  std::is_same< remove_cvptr_t<U>, remove_cvptr_t<T> >::value };
};

template<class ...Args>
using common_type_t = typename std::common_type<Args...>::type;

} // namespace util
} // namespace borealis

#endif /* TYPE_TRAITS_HPP_ */
