//
// Created by abdullin on 4/7/17.
//

#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Pointer.h"
#include "Util.h"
#include "Util/collections.hpp"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Pointer::Pointer(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType)
        : Domain{value, POINTER, factory},
          elementType_(elementType) {}

Pointer::Pointer(DomainFactory* factory, const llvm::Type& elementType, const Pointer::Locations& locations)
        : Domain{VALUE, POINTER, factory},
          elementType_(elementType),
          locations_(locations) {}

Pointer::Pointer(const Pointer& other)
        : Domain{other.value_, other.type_, other.factory_},
          elementType_(other.elementType_),
          locations_(other.locations_) {}

Domain& Pointer::operator=(const Domain& other) {
    auto&& ptr = llvm::dyn_cast<Pointer>(&other);
    ASSERT(ptr, "Nullptr in Pointer join");
    if (this == ptr) return *this;

    Domain::operator=(other);
    ASSERT(elementType_.getTypeID() == ptr->elementType_.getTypeID(), "Different types");
    locations_ = ptr->locations_;
    return *this;
}

bool Pointer::equals(const Domain* other) const {
    auto&& ptr = llvm::dyn_cast<Pointer>(other);
    if (not ptr) return false;
    if (this == other) return true;

    if (this->isBottom() && other->isBottom()) return true;
    if (this->isTop() && other->isTop()) return true;

    return elementType_.getTypeID() == ptr->elementType_.getTypeID() &&
            locations_ == ptr->locations_;
}

bool Pointer::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

const Pointer::Locations& Pointer::getLocations() const {
    return locations_;
}

const llvm::Type& Pointer::getElementType() const {
    return elementType_;
}

size_t Pointer::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getElementType().getTypeID(), getLocations());
}

std::string Pointer::toString() const {
    std::ostringstream ss;
    ss << "[";
    for (auto&& it : locations_) {
        if (it) ss << std::endl << it->toString();
    }
    ss << std::endl << "]";
    return ss.str();
}

Domain* Pointer::clone() const {
    return new Pointer(*this);
}

bool Pointer::classof(const Domain* other) {
    return other->getType() == Domain::POINTER;
}

Domain::Ptr Pointer::join(Domain::Ptr other) const {
    auto&& ptr = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(ptr, "Non-pointer in pointer domain");
    for (auto&& it : locations_) {
        for (auto&& itptr : ptr->locations_) {
            if (it) it = it->join(itptr);
        }
    }
    return shared_from_this();
}

Domain::Ptr Pointer::widen(Domain::Ptr other) const {
    auto&& ptr = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(ptr, "Non-pointer in pointer domain");
    for (auto&& it : locations_) {
        for (auto&& itptr : ptr->locations_) {
            if (it) it = it->widen(itptr);
        }
    }
    return shared_from_this();
}

Domain::Ptr Pointer::meet(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Pointer::narrow(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Pointer::load(const llvm::Type& type, const std::vector<Domain::Ptr>& offsets) const {
    auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
    if (not intOffset->isConstant(0)) {
        errs() << "big offset" << endl;
        return factory_->getTop(type);
    }

    if (isBottom())
        return factory_->getBottom(type);
    else if (isTop())
        return factory_->getTop(type);

    auto result = factory_->getBottom(elementType_);
    if (offsets.size() == 1) {
        ASSERT(elementType_.getTypeID() == type.getTypeID(), "Different load types");
        for (auto&& it : locations_)
            if (it) result = result->join(it);

    } else {
        std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
        for (auto&& it : locations_)
            if (it) result = result->join(it->load(type, subOffsets));
    }
    return result;
}

void Pointer::store(Domain::Ptr value, const std::vector<Domain::Ptr>& offsets) const {
    auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
    ASSERT(intOffset->isConstant(0), "Too big offset in pointer");

    if (offsets.size() == 1) {
        for (auto&& it : locations_) {
            // This is generally fucked up
            if (it) {
                auto withoutConst = const_cast<Domain*>(it.get());
                *withoutConst = *(it->join(value).get());
            }
        }

    } else {
        std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
        for (auto&& it : locations_)
            if (it) it->store(value, subOffsets);
    }
}

Domain::Ptr Pointer::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const {
    auto&& intOffset = llvm::dyn_cast<IntegerInterval>(indices.begin()->get());
    if (not intOffset->isConstant(0)) {
        return factory_->getTop(type);
    }

    if (isBottom())
        return factory_->getBottom(type);
    else if (isTop())
        return factory_->getTop(type);

    std::vector<Domain::Ptr> newLocations;
    if (indices.size() == 1) {
        ASSERT(elementType_.getTypeID() == type.getTypeID(), "Different load types");
        for (auto&& it : locations_)
            if (it) newLocations.push_back(it);

    } else {
        std::vector<Domain::Ptr> subOffsets(indices.begin() + 1, indices.end());
        for (auto&& it : locations_) {
            if (it) {
                auto gepResult = it->gep(type, subOffsets);
                auto ptr = llvm::dyn_cast<Pointer>(gepResult.get());
                util::viewContainer(ptr->getLocations()).foreach([&newLocations](Domain::Ptr loc) {
                    newLocations.push_back(loc);
                });
            }
        }
    }
    return factory_->getPointer(*type.getPointerElementType(), newLocations);
}

Domain::Ptr Pointer::ptrtoint(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr Pointer::bitcast(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr Pointer::icmp(Domain::Ptr, llvm::CmpInst::Predicate) const {
    return factory_->getInteger(TOP, 1, false);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

