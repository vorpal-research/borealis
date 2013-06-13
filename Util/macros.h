/*
 * macros.h
 *
 *  Created on: Dec 7, 2012
 *      Author: belyaev
 */

// GUARDS ARE NOT USED FOR A REASON!
// #ifndef MACROS_H_
// #define MACROS_H_

#ifdef BOREALIS_MACROS_DEFINED
#error "macros.h included twice!"
#endif

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
 * */
#define QUICK_RETURN(...) ->decltype(__VA_ARGS__) { return __VA_ARGS__; }

#define QUICK_CONST_RETURN(...) const ->decltype(__VA_ARGS__) { return __VA_ARGS__; }

#define BYE_BYE(type, msg) return borealis::util::sayonara<type>( \
        __FILE__, \
        __LINE__, \
        __PRETTY_FUNCTION__, \
        msg)

#define BYE_BYE_VOID(msg) { \
        borealis::util::sayonara<void>( \
            __FILE__, \
            __LINE__, \
            __PRETTY_FUNCTION__, \
            msg); \
        return; \
    }

#define ASSERT(cond, msg) while(!(cond)){ borealis::util::sayonara( \
        __FILE__, \
        __LINE__, \
        __PRETTY_FUNCTION__, \
        msg); }

#define ASSERTC(cond) while(!(cond)){ borealis::util::sayonara( \
        __FILE__, \
        __LINE__, \
        __PRETTY_FUNCTION__, \
        #cond); }

#define GUARD(...) typename std::enable_if<(__VA_ARGS__)>::type
#define GUARDED(TYPE, ...) typename std::enable_if<(__VA_ARGS__), TYPE>::type

/*[[[cog
import cog
maxElems = 80

cog.outl('''\
#define STATIC_STRING(S) \\
    borealis::util::make_ss< \\\
''')
cog.outl(\
',\\\n'.join(["borealis::util::at(S,%2d)"%i for i in range(maxElems)]) + " \\" \
)
cog.outl('''\
>
''')
]]]*/
#define STATIC_STRING(S) \
    borealis::util::make_ss< \
borealis::util::at(S, 0),\
borealis::util::at(S, 1),\
borealis::util::at(S, 2),\
borealis::util::at(S, 3),\
borealis::util::at(S, 4),\
borealis::util::at(S, 5),\
borealis::util::at(S, 6),\
borealis::util::at(S, 7),\
borealis::util::at(S, 8),\
borealis::util::at(S, 9),\
borealis::util::at(S,10),\
borealis::util::at(S,11),\
borealis::util::at(S,12),\
borealis::util::at(S,13),\
borealis::util::at(S,14),\
borealis::util::at(S,15),\
borealis::util::at(S,16),\
borealis::util::at(S,17),\
borealis::util::at(S,18),\
borealis::util::at(S,19),\
borealis::util::at(S,20),\
borealis::util::at(S,21),\
borealis::util::at(S,22),\
borealis::util::at(S,23),\
borealis::util::at(S,24),\
borealis::util::at(S,25),\
borealis::util::at(S,26),\
borealis::util::at(S,27),\
borealis::util::at(S,28),\
borealis::util::at(S,29),\
borealis::util::at(S,30),\
borealis::util::at(S,31),\
borealis::util::at(S,32),\
borealis::util::at(S,33),\
borealis::util::at(S,34),\
borealis::util::at(S,35),\
borealis::util::at(S,36),\
borealis::util::at(S,37),\
borealis::util::at(S,38),\
borealis::util::at(S,39),\
borealis::util::at(S,40),\
borealis::util::at(S,41),\
borealis::util::at(S,42),\
borealis::util::at(S,43),\
borealis::util::at(S,44),\
borealis::util::at(S,45),\
borealis::util::at(S,46),\
borealis::util::at(S,47),\
borealis::util::at(S,48),\
borealis::util::at(S,49),\
borealis::util::at(S,50),\
borealis::util::at(S,51),\
borealis::util::at(S,52),\
borealis::util::at(S,53),\
borealis::util::at(S,54),\
borealis::util::at(S,55),\
borealis::util::at(S,56),\
borealis::util::at(S,57),\
borealis::util::at(S,58),\
borealis::util::at(S,59),\
borealis::util::at(S,60),\
borealis::util::at(S,61),\
borealis::util::at(S,62),\
borealis::util::at(S,63),\
borealis::util::at(S,64),\
borealis::util::at(S,65),\
borealis::util::at(S,66),\
borealis::util::at(S,67),\
borealis::util::at(S,68),\
borealis::util::at(S,69),\
borealis::util::at(S,70),\
borealis::util::at(S,71),\
borealis::util::at(S,72),\
borealis::util::at(S,73),\
borealis::util::at(S,74),\
borealis::util::at(S,75),\
borealis::util::at(S,76),\
borealis::util::at(S,77),\
borealis::util::at(S,78),\
borealis::util::at(S,79) \
>

//[[[end]]]

// #endif /* MACROS_H_ */
