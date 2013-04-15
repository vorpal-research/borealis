/*
 * unmacros.h
 *
 *  Created on: Dec 7, 2012
 *      Author: belyaev
 */

// GUARDS ARE NOT USED FOR A REASON!
// #ifndef UNMACROS_H_
// #define UNMACROS_H_

#ifdef BOREALIS_MACROS_DEFINED
#undef QUICK_RETURN
#undef QUICK_CONST_RETURN
#undef BYE_BYE
#undef BYE_BYE_VOID
#undef ASSERT
#undef ASSERTC
#undef GUARD
#undef GUARDED
#undef STATIC_STRING
#undef BOREALIS_MACROS_DEFINED
#else
#error "unmacros.h is included without corresponding macros.h include!"
#endif // BOREALIS_MACROS_DEFINED

// #endif /* UNMACROS_H_ */
