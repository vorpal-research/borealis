/*
 * ReturnValueTerm.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include "Term/ReturnValueTerm.h"

namespace borealis {

ReturnValueTerm::ReturnValueTerm(Type::Ptr type, const std::string& funcName) :
    Term(
        class_tag(*this),
        type,
        "\\result_" + funcName
    ), funcName(funcName) {};

const std::string& ReturnValueTerm::getFunctionName() const {
    return funcName;
}

} // namespace borealis
