/*
 * TypeFactory.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#include "Type/TypeFactory.h"

namespace borealis {

TypeFactory::TypeFactory() {}

TypeFactory::~TypeFactory() {}

std::ostream& operator<<(std::ostream& ost, Type::Ptr tp) {
    return ost << TypeFactory::getInstance().toString(*tp);
}

} /* namespace borealis */
