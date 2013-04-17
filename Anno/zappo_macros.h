/*
 * zappo_macros.h
 *
 *  Created on: Apr 16, 2013
 *      Author: belyaev
 */

#ifndef ZAPPO_HPP_
#error "You should include zappo.hpp before zappo_macros.h"
#endif

#ifdef ZAPPO_MACROS_DEFINED
#error "zappo_macros.h included twice!"
#endif

// GUARDS NOT USED FOR A REASON!
//#ifndef ZAPPO_MACROS_H_
//#define ZAPPO_MACROS_H_
#define ZAPPO_MACROS_DEFINED

#include "Util/macros.h"
#define _PS(STR) typename borealis::zappo::static_string2pegtl_string<STATIC_STRING(STR)>::type

#define G(...) borealis::zappo::tag<__VA_ARGS__>()
#define S(STR) borealis::zappo::tag<_PS(STR)>()
#define PSS(STR) borealis::zappo::tag<STR>()

#define GRAMMAR(...) borealis::zappo::untag<decltype((__VA_ARGS__))>
#define LITERALGRAMMAR(...) pegtl::pad<borealis::zappo::untag<decltype((__VA_ARGS__))>, pegtl::space>
#define CH(...) G(pegtl::one<__VA_ARGS__>)
#define RANGE(from, to) G(pegtl::range<from, to>)

//#endif /* ZAPPO_MACROS_H_ */
