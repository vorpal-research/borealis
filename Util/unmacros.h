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

/*[[[cog
import cog
import re

with open("Util/macros.h") as f:
    for line in f:
        if(line.startswith("#define ")):
            macro = line.replace(' ', '(').split('(',2)[1]
            if(re.match('^\w+(\w\d)?$', macro)):
                cog.outl("#undef %s" % macro)
]]]*/
#undef BOREALIS_MACROS_DEFINED

#undef LAM
#undef LAM2
#undef FWD
#undef APPLY
#undef QUICK_RETURN
#undef QUICK_CONST_RETURN
#undef BYE_BYE
#undef BYE_BYE_VOID
#undef ASSERT
#undef ASSERTC
#undef UNREACHABLE
#undef GUARD
#undef GUARDED
#undef STATIC_STRING
#undef STATIC_STRING
#undef NORETURN
#undef PRETOKENPASTE
#undef TOKENPASTE
#undef ON_SCOPE_EXIT
#undef NULLPTRIFY1
#undef NULLPTRIFY2
#undef NULLPTRIFY3
#undef NULLPTRIFY4
#undef NULLPTRIFY5
#undef NULLPTRIFY6
#undef NULLPTRIFY7
#undef COMPILER
#undef COMPILER
#undef DECLTYPE_AUTO
#undef DECLTYPE_AUTO
//[[[end]]]


#else
#error "unmacros.h is included without corresponding macros.h include!"
#endif // BOREALIS_MACROS_DEFINED

// #endif /* UNMACROS_H_ */
