//
// Created by ice-phoenix on 10/31/15.
//

#ifndef SANDBOX_MEMORY_H
#define SANDBOX_MEMORY_H

#include "Reanimator/MemoryObject.h"
#include <Util/util.h>

#include "Util/macros.h"

namespace borealis {

class Memory {

public:
    const std::map<unsigned long long, MemoryObject>& getStorage() const {
        return storage;
    }

    Memory& add(unsigned long long addr, const MemoryObject& obj) {
        ASSERTC(not util::contains(storage, addr) || storage.at(addr) == obj);
        storage.emplace(addr, obj);
        return *this;
    }

private:
    std::map<unsigned long long, MemoryObject> storage;
};

} // namespace borealis

#include "Util/unmacros.h"

#endif //SANDBOX_MEMORY_H
