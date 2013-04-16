/*
 * zappo.hpp
 *
 *  Created on: Oct 16, 2012
 *      Author: belyaev
 */

#ifndef ZAPPO_HPP_
#define ZAPPO_HPP_

#include "anno.h"

namespace borealis {
namespace zappo {

template<class SS> struct static_string2pegtl_string;
template<char ...SS> struct static_string2pegtl_string<util::static_string<SS...>> {
    typedef pegtl::string<SS...> type;
};

// pegtl::string -> pstring
template<int ...Chars>
struct pstring : pegtl::string< Chars... > {};

template<typename T>
struct tag {
    typedef T type;
};

template<class T>
using untag = typename T::type;

template<class T>          tag<pegtl::star    <T>   > operator * (tag<T>);
template<class T>          tag<pegtl::plus    <T>   > operator + (tag<T>);
template<class T>          tag<pegtl::opt     <T>   > operator ~ (tag<T>);
template<class T, class U> tag<pegtl::sor     <T, U>> operator | (tag<T>, tag<U>);
template<class T, class U> tag<pegtl::seq     <T, U>> operator >> (tag<T>, tag<U>);
template<class T, class U> tag<pegtl::ifmust  <T, U>> operator >= (tag<T>, tag<U>);
template<class T, class U> tag<pegtl::ifapply <T, U>> operator ^ (tag<T>, tag<U>);




}// namespace zappo
}// namespace borealis

#endif /* ZAPPO_HPP_ */
