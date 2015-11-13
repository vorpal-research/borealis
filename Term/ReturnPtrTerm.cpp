/*
 * ReturnPtrTerm.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include "Term/ReturnPtrTerm.h"

namespace borealis {

ReturnPtrTerm::ReturnPtrTerm(Type::Ptr type, const std::string& funcName) :
    Term(
        class_tag(*this),
        type,
        "&\\result_" + funcName
    ), funcName(funcName) {};

const std::string& ReturnPtrTerm::getFunctionName() const {
    return funcName;
}

} // namespace borealis
