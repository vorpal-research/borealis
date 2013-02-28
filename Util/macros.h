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
 * Macro for quick-writing one-liners with tricky typing.
 * This can be used to replace this (note the same `a+b` used twice):
 *
 * template<class A, class B>
 * auto plus(A a, B b) -> decltype(a+b) { return a+b; }
 *
 * with this:
 *
 * template<class A, class B>
 * auto plus(A a, B b) QUICK_RETURN(a+b)
 *
 * Note that the one-liners can be big and the impact will be significant.
 *
 * TODO: the name choice is poor
 *
 * */
#define QUICK_RETURN(...) ->decltype(__VA_ARGS__) { return __VA_ARGS__; }

#define QUICK_CONST_RETURN(...) const ->decltype(__VA_ARGS__) { return __VA_ARGS__; }

#define BYE_BYE(type, msg) return borealis::util::sayonara<type>( \
        __FILE__, \
        __LINE__, \
        __PRETTY_FUNCTION__, \
        msg);

#define BYE_BYE_VOID(msg) { \
        borealis::util::sayonara<void>( \
            __FILE__, \
            __LINE__, \
            __PRETTY_FUNCTION__, \
            msg); \
        return; \
    }

#define ASSERT(cond, msg) while(!cond){ borealis::util::sayonara( \
        __FILE__, \
        __LINE__, \
        __PRETTY_FUNCTION__, \
        msg);}

#define ASSERTC(cond) while(!(cond)){ borealis::util::sayonara( \
        __FILE__, \
        __LINE__, \
        __PRETTY_FUNCTION__, \
        #cond);}

// #endif /* MACROS_H_ */
