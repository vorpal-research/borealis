//
// Created by abdullin on 4/7/17.
//

#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Pointer.h"
#include "Interpreter/Util.h"
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

bool Pointer::equals(const Domain* other) const {
    auto ptr = llvm::dyn_cast<Pointer>(other);
    if (not ptr) return false;
    if (this == ptr) return true;

    if (locations_.size() != ptr->locations_.size()) return false;

    for (auto&& it : locations_) {
        auto&& itptr = ptr->locations_.find(it);
        if (itptr == ptr->locations_.end()) return false;
        if (not itptr->location_->equals(it.location_.get())) return false;
    }
    return true;
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

std::size_t Pointer::hashCode() const {
    return 0;
    //return util::hash::simple_hash_value(value_, getType(), getElementType().getTypeID(), getLocations());
}

std::string Pointer::toString() const {
    std::ostringstream ss;
    ss << "Ptr " << util::toString(elementType_) << " [";
    if (isTop()) ss << " TOP ]" << std::endl;
    else if (isBottom()) ss << " BOTTOM ]" << std::endl;
    else {
        for (auto&& it : locations_) {
            ss << std::endl << it.offset_->toString() << " " << it.location_->toString();
        }
        ss << std::endl << "]";
    }
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
    ASSERT(ptr, "Non-pointer domain in pointer join");

    if (other->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return other->shared_from_this();
    } else if (other->isTop() || this->isTop()) {
        return factory_->getPointer(TOP, elementType_);
    }

    for (auto&& itptr : ptr->locations_) {
        auto&& it = locations_.find(itptr);
        if (it == locations_.end()) {
            locations_.insert(itptr);
        } else {
            /// Assume that length and location are same
            it->offset_ = it->offset_->join(itptr.offset_);
        }
    }
    return shared_from_this();
}

Domain::Ptr Pointer::widen(Domain::Ptr other) const {
    return join(other);
}

Domain::Ptr Pointer::meet(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Pointer::narrow(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Pointer::load(const llvm::Type& type, Domain::Ptr offset) const {
    if (isBottom()) {
        return factory_->getBottom(type);
    } else if (isTop()) {
        return factory_->getTop(type);
    }

    auto result = factory_->getBottom(type);
    for (auto&& it : locations_) {
        auto totalOffset = it.offset_->add(offset);
        result = result->join(it.location_->load(type, totalOffset));
    }
    return result;
}

void Pointer::store(Domain::Ptr value, Domain::Ptr offset) const {
    if (isBottom() || isTop()) return;

    if (elementType_.isPointerTy()) {
        locations_.clear();
        locations_.insert({factory_->getIndex(0), value});
    } else {
        for (auto&& it : locations_) {
            auto totalOffset = it.offset_->add(offset);
            it.location_->store(value, {totalOffset});
        }
    }
}

Domain::Ptr Pointer::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const {
    if (isBottom() || isTop()) {
        return factory_->getPointer(TOP, type);
    }

    auto result = factory_->getPointer(BOTTOM, type);
    std::vector<Domain::Ptr> subOffsets(indices.begin(), indices.end());
    auto zeroElement = subOffsets[0];

    for (auto&& it : locations_) {
        subOffsets[0] = zeroElement->add(it.offset_);
        result = result->join(it.location_->gep(type, subOffsets));
    }
    return result;
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

