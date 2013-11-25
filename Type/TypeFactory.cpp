/*
 * TypeFactory.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#include "Type/TypeFactory.h"
#include "Type/TypeUtils.h"

namespace borealis {

TypeFactory::TypeFactory() : recordBodies{ new type::RecordRegistry{} } {}

std::ostream& operator<<(std::ostream& ost, const Type& tp) {
    return ost << TypeUtils::toString(tp);
}

} /* namespace borealis */
