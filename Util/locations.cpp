/*
 * locations.cpp
 *
 *  Created on: Oct 9, 2012
 *      Author: belyaev
 */

#include "locations.h"

namespace borealis {

std::ostream& operator<<(std::ostream& ost, const Locus& ll) {
    // file.txt:1:2
    return ost << ll.filename << ":" << ll.loc;
}

} // namespace borealis
