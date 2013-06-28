/*
 * DefectInfo.cpp
 *
 *  Created on: Apr 15, 2013
 *      Author: ice-phoenix
 */

#include "Passes/Defect/DefectManager/DefectInfo.h"

namespace borealis {

bool operator==(const DefectInfo& a, const DefectInfo& b) {
    return a.type == b.type && a.location == b.location;
}

bool operator<(const DefectInfo& a, const DefectInfo& b) {
    if (a.type < b.type) return true;
    if (a.type > b.type) return false;
    if (a.location < b.location) return true;
    if (a.location > b.location) return false;
    return false;
}

std::ostream& operator<<(std::ostream& s, const DefectInfo& di) {
    return s << DefectTypeNames.at(di.type).type << ":" << di.location;
}

} // namespace borealis
