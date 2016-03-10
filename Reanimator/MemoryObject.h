//
// Created by ice-phoenix on 10/31/15.
//

#ifndef SANDBOX_MEMORYOBJECT_H
#define SANDBOX_MEMORYOBJECT_H

#include <Type/Type.h>
#include <Util/util.h>

#include "Util/macros.h"

namespace borealis {

class MemoryObject {

public:
    MemoryObject(Type::Ptr type, util::option<int64_t> value, unsigned long long size)
        : type(type), value(value), size(size) { }

    MemoryObject(Type::Ptr type, unsigned long long size)
        : MemoryObject(type, util::nothing(), size) { }

    MemoryObject(Type::Ptr type, int64_t value, unsigned long long size)
        : MemoryObject(type, util::just(value), size) { }

    Type::Ptr getType() const {
        return type;
    }

    util::option<int64_t> getValue() const {
        return value;
    }

    unsigned long long getSize() const {
        return size;
    }

    const std::map<unsigned long long, MemoryObject>& getNested() const {
        return nested;
    }

    MemoryObject& add(unsigned long long shift, const MemoryObject& obj) {
        ASSERTC(not util::contains(nested, shift) || nested.at(shift) == obj);
        nested.emplace(shift, obj);
        return *this;
    }

    friend bool operator==(const MemoryObject& a, const MemoryObject& b);

private:
    Type::Ptr type;
    util::option<int64_t> value;

    unsigned long long size;
    std::map<unsigned long long, MemoryObject> nested;
};

std::ostream& operator<<(std::ostream& s, const MemoryObject& mo);

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const MemoryObject& mo);

} // namespace borealis

#include "Util/unmacros.h"

#endif //SANDBOX_MEMORYOBJECT_H
