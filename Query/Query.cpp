/*
 * Query.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: ice-phoenix
 */

#include "Query/Query.h"

namespace borealis {

Query::~Query() {}

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Query& q) {
    s << q.toString();
    return s;
}

std::ostream& operator<<(std::ostream& s, const borealis::Query& q) {
    s << q.toString();
    return s;
}

} // namespace borealis
