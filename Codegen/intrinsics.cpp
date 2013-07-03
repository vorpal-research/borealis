/*
 * intrinsics.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include "Codegen/intrinsics.h"

using namespace borealis;

bool borealis::isIntrinsic(function_type ft) {
    return function_type::FIRST_INTRINSIC < ft && ft < function_type::LAST_INTRINSIC;
}

bool borealis::isBuiltin(function_type ft) {
    return function_type::FIRST_BUILTIN < ft && ft < function_type::LAST_BUILTIN;
}

bool borealis::isAction(function_type ft) {
    return function_type::FIRST_ACTION < ft && ft < function_type::LAST_ACTION;
}

bool borealis::isUnknown(function_type ft) {
    return ft == function_type::UNKNOWN;
}
