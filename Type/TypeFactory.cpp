/*
 * TypeFactory.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#include "Type/TypeFactory.h"

namespace borealis {

TypeFactory::TypeFactory() : recordBodies{ new type::RecordRegistry{} } {}

std::ostream& operator<<(std::ostream& ost, Type::Ptr tp) {
    static TypeFactory::Ptr TyF{ TypeFactory::get() };
    return ost << TyF->toString(*tp);
}

} /* namespace borealis */
