/*
 * intrinsics.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef INTRINSICS_H_
#define INTRINSICS_H_

#include "Util/util.h"

namespace borealis {

enum class function_type {
    FIRST_INTRINSIC,
    INTRINSIC_PTR_VERSION,
    INTRINSIC_VALUE,
    INTRINSIC_DECLARE,
    INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
    INTRINSIC_GLOBAL,
    INTRINSIC_ANNOTATION,
    INTRINSIC_MALLOC,
    INTRINSIC_ALLOC,
    INTRINSIC_NONDET,
    INTRINSIC_CONSUME,
    LAST_INTRINSIC,

    FIRST_BUILTIN,
    BUILTIN_BOR_ASSERT,
    BUILTIN_BOR_ASSUME,
    BUILTIN_BOR_GETPROP,
    BUILTIN_BOR_SETPROP,
    LAST_BUILTIN,

    FIRST_ACTION,
    ACTION_DEFECT,
    LAST_ACTION,

    UNKNOWN
};

bool isIntrinsic(function_type ft);
bool isBuiltin(function_type ft);
bool isAction(function_type ft);
bool isUnknown(function_type ft);

template<class Streamer>
Streamer& operator << (Streamer& str, function_type ft) {
    switch(ft) {
    case function_type::FIRST_INTRINSIC:                   str << "function_type::FIRST_INTRINSIC"; break;
    case function_type::INTRINSIC_PTR_VERSION:             str << "function_type::INTRINSIC_PTR_VERSION"; break;
    case function_type::INTRINSIC_VALUE:                   str << "function_type::INTRINSIC_VALUE"; break;
    case function_type::INTRINSIC_DECLARE:                 str << "function_type::INTRINSIC_DECLARE"; break;
    case function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE: str << "function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE"; break;
    case function_type::INTRINSIC_GLOBAL:                  str << "function_type::INTRINSIC_GLOBAL"; break;
    case function_type::INTRINSIC_ANNOTATION:              str << "function_type::INTRINSIC_ANNOTATION"; break;
    case function_type::INTRINSIC_MALLOC:                  str << "function_type::INTRINSIC_MALLOC"; break;
    case function_type::INTRINSIC_ALLOC:                   str << "function_type::INTRINSIC_ALLOC"; break;
    case function_type::INTRINSIC_NONDET:                  str << "function_type::INTRINSIC_NONDET"; break;
    case function_type::INTRINSIC_CONSUME:                 str << "function_type::INTRINSIC_CONSUME"; break;
    case function_type::LAST_INTRINSIC:                    str << "function_type::LAST_INTRINSIC"; break;

    case function_type::FIRST_BUILTIN:                     str << "function_type::FIRST_BUILTIN"; break;
    case function_type::BUILTIN_BOR_ASSERT:                str << "function_type::BUILTIN_BOR_ASSERT"; break;
    case function_type::BUILTIN_BOR_ASSUME:                str << "function_type::BUILTIN_BOR_ASSUME"; break;
    case function_type::BUILTIN_BOR_GETPROP:               str << "function_type::BUILTIN_BOR_GETPROP"; break;
    case function_type::BUILTIN_BOR_SETPROP:               str << "function_type::BUILTIN_BOR_SETPROP"; break;
    case function_type::LAST_BUILTIN:                      str << "function_type::LAST_BUILTIN"; break;

    case function_type::FIRST_ACTION:                      str << "function_type::FIRST_ACTION"; break;
    case function_type::ACTION_DEFECT:                     str << "function_type::ACTION_DEFECT"; break;
    case function_type::LAST_ACTION:                       str << "function_type::LAST_ACTION"; break;

    case function_type::UNKNOWN:                           str << "function_type::UNKNOWN"; break;
    }
    return str;
}

} // namespace borealis

namespace std {
template<>
struct hash<borealis::function_type> : public borealis::util::enums::enum_hash<borealis::function_type> {};
} // namespace std

#endif /* INTRINSICS_H_ */
