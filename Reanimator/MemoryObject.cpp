//
// Created by ice-phoenix on 10/31/15.
//

#include "Reanimator/MemoryObject.h"
#include <Type/TypeUtils.h>

namespace borealis {

bool operator==(const MemoryObject& a, const MemoryObject& b) {
    return a.type == b.type
           && a.value == b.value
           && a.size == b.size
           && a.nested == b.nested;
}

std::ostream& operator<<(std::ostream& s, const MemoryObject& mo) {
    s << "(" << TypeUtils::toString(*mo.getType()) << ") ";
    s << mo.getValue();
    s << " [" << mo.getSize() << "]";
    if (not mo.getNested().empty()) {
        s << ":";
        for (auto&& e : mo.getNested()) {
            s << std::endl << e.first << "->" << e.second;
        }
    }
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const MemoryObject& mo) {
    s << "(" << TypeUtils::toString(*mo.getType()) << ") ";
    s << mo.getValue();
    s << " [" << mo.getSize() << "]";
    if (not mo.getNested().empty()) {
        s << ":" << logging::il;
        for (auto&& e : mo.getNested()) {
            s << logging::endl << e.first << "->" << e.second;
        }
        s << logging::ir;
    }
    return s;
}

} // namespace borealis
