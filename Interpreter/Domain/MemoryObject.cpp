//
// Created by abdullin on 4/14/17.
//

#include "MemoryObject.h"

namespace borealis {
namespace absint {

MemoryObject::MemoryObject(Domain::Ptr content) : content_(content) {}

Domain::Ptr MemoryObject::load() const {
    return content_;
}

void MemoryObject::store(Domain::Ptr newContent) const {
    content_ = content_->join(newContent);
}

size_t MemoryObject::hashCode() const {
    return content_->hashCode();
}

std::string MemoryObject::toString(const std::string prefix) const {
    std::ostringstream ss;
    ss << "@MO: " << content_->toString(prefix);
    return ss.str();
}

bool MemoryObject::equals(const MemoryObject* other) const {
    if (this == other) return true;
    return content_->equals(other->load().get());
}

}   /* namespace absint */
}   /* namespace borealis */