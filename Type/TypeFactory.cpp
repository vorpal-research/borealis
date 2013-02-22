/*
 * TypeFactory.cpp
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#include "TypeFactory.h"

namespace borealis {

TypeFactory::TypeFactory() {
    // TODO Auto-generated constructor stub

}

TypeFactory::~TypeFactory() {
    // TODO Auto-generated destructor stub
}

std::ostream& operator<<(std::ostream& ost, const Type& tp) {
    return ost << TypeFactory::getInstance().toString(tp);
}

} /* namespace borealis */
