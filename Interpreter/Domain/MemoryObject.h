//
// Created by abdullin on 4/14/17.
//

#ifndef BOREALIS_MEMORYOBJECT_H
#define BOREALIS_MEMORYOBJECT_H

#include <memory>
#include <string>

#include "Domain.h"

namespace borealis {
namespace absint {

class MemoryObject : public std::enable_shared_from_this<const MemoryObject> {
public:

    using Ptr = std::shared_ptr<const MemoryObject>;

    MemoryObject(Domain::Ptr content);

    Domain::Ptr load() const;
    void store(Domain::Ptr newContent) const;

    bool equals(const MemoryObject* other) const;
    size_t hashCode() const;
    std::string toString() const;

private:

    mutable Domain::Ptr content_;

};

struct MemoryObjectHash {
    size_t operator()(MemoryObject::Ptr mo) const noexcept {
        return mo->hashCode();
    }
};

struct MemoryObjectEquals {
    bool operator()(MemoryObject::Ptr lhv, MemoryObject::Ptr rhv) const {
        return lhv->equals(rhv.get());
    }
};

}   /* namespace absint */
}   /* namespace borealis */


#endif //BOREALIS_MEMORYOBJECT_H
