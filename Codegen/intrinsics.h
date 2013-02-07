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
    LAST_INTRINSIC,

    FIRST_BUILTIN,
    BUILTIN_MALLOC,
    LAST_BUILTIN,

    UNKNOWN
};

bool isIntrinsic(function_type ft);
bool isBuiltin(function_type ft);
bool isUnknown(function_type ft);

} // namespace borealis

namespace std {
template<>
struct hash<borealis::function_type> : public borealis::util::enums::enum_hash<borealis::function_type> {};
} // namespace std

#endif /* INTRINSICS_H_ */
