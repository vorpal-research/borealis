/*
 * macros.h
 *
 *  Created on: Dec 7, 2012
 *      Author: belyaev
 */

// GUARDS ARE NOT USED FOR A REASON!
// #ifndef MACROS_H_
// #define MACROS_H_

#define BOREALIS_MACROS_DEFINED

/*
 * Macro for quick-writing of one-liners with tricky typing.
 * This can be used to replace this (note the clone):
 *
 * template<class A, class B>
 * auto plus(A a, B b) -> decltype(a+b) { return a+b; }
 *
 * with this:
 *
 * template<class A, class B>
 * auto plus(A a, B b) QUICK_RETURN(a+b)
 *
 * Note that one-liners can be big and the impact will be significant.
 * TODO: the name choice is poor
 *
 * */
#define QUICK_RETURN(...) ->decltype(__VA_ARGS__) { return __VA_ARGS__; }



// #endif /* MACROS_H_ */