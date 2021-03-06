/*
 * intrinsics.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef INTRINSICS_H_
#define INTRINSICS_H_

#include "Util/util.h"

#include "Util/generate_macros.h"

namespace borealis {

GENERATE_FANCY_ENUM(
    function_type,
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
    INTRINSIC_UNREACHABLE,
    INTRINSIC_CALL_AND_STORE,
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
);

bool isIntrinsic(function_type ft);
bool isBuiltin(function_type ft);
bool isAction(function_type ft);
bool isUnknown(function_type ft);

} // namespace borealis

namespace std {
template<>
struct hash<borealis::function_type> : public borealis::util::enums::enum_hash<borealis::function_type> {};
} // namespace std

#include "Util/generate_unmacros.h"

#endif /* INTRINSICS_H_ */
