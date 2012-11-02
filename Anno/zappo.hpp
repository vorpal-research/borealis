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

template<typename T>
struct tag {
    typedef T type;
};

template<class T>          tag<pegtl::star    <T>   > operator * (tag<T>);
template<class T>          tag<pegtl::plus    <T>   > operator + (tag<T>);
template<class T, class U> tag<pegtl::sor     <T, U>> operator | (tag<T>, tag<U>);
template<class T, class U> tag<pegtl::seq     <T, U>> operator >>(tag<T>, tag<U>);
template<class T, class U> tag<pegtl::ifapply <T, U>> operator ^ (tag<T>, tag<U>);

}// namespace zappo
}// namespace borealis

#endif /* ZAPPO_HPP_ */
