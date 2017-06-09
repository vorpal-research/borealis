//
// Created by abdullin on 4/7/17.
//

#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Pointer.h"
#include "Interpreter/Util.h"
#include "Util/collections.hpp"
#include "Util/hash.hpp"
#include "Util/streams.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Pointer::Pointer(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType, bool isNullptr)
        : Domain{value, POINTER, factory},
          elementType_(elementType),
          nullptr_(isNullptr) {}

Pointer::Pointer(DomainFactory* factory, const llvm::Type& elementType, const Pointer::Locations& locations)
        : Domain{VALUE, POINTER, factory},
          elementType_(elementType),
          nullptr_(false),
          locations_(locations) {
    for (auto&& it : locations) {
        if (it.offset_->isTop() || it.location_->isTop()) {
            value_ = TOP;
            locations_.clear();
            break;
        }
    }
}

bool Pointer::equals(const Domain* other) const {
    auto ptr = llvm::dyn_cast<Pointer>(other);
    if (not ptr) return false;
    if (this == ptr) return true;

    if (this->isNullptr() && ptr->isNullptr()) return true;
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

bool Pointer::isNullptr() const {
    return nullptr_;
}

const Pointer::Locations& Pointer::getLocations() const {
    return locations_;
}

const llvm::Type& Pointer::getElementType() const {
    return elementType_;
}

std::size_t Pointer::hashCode() const {
    return util::hash::simple_hash_value(value_, type_, /*elementType_.getTypeID(), */nullptr_);
}

std::string Pointer::toString(const std::string prefix) const {
    std::ostringstream ss;
    ss << "Ptr " << util::toString(elementType_) << " [";
    if (isNullptr()) ss << " nullptr ]";
    else if (isTop()) ss << " TOP ]";
    else if (isBottom()) ss << " BOTTOM ]";
    else {
        for (auto&& it : locations_) {
            ss << std::endl << prefix << "  " << it.offset_->toString() << " " << it.location_->toString(prefix + "  ");
        }
        ss << std::endl << prefix << "]";
    }
    return ss.str();
}

bool Pointer::classof(const Domain* other) {
    return other->getType() == Domain::POINTER;
}

Domain::Ptr Pointer::join(Domain::Ptr other) const {
    auto&& ptr = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    if (this == other.get()) return shared_from_this();
    if (this->isNullptr() || ptr->isNullptr())
        return factory_->getPointer(TOP, elementType_);

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
    if (not isValue())
        return factory_->getTop(type);
    if (isNullptr()) {
        errs() << "Load from nullptr" << endl;
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
    if (isNullptr()) {
        errs() << "Store to nullptr" << endl;
        return;
    }

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
    if (isNullptr()) {
        errs() << "GEP from nullptr" << endl;
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
    auto intType = llvm::cast<llvm::IntegerType>(&type);
    if (isNullptr()) return factory_->getInteger(factory_->toInteger(0, intType->getBitWidth()));
    return factory_->getTop(type);
}

Domain::Ptr Pointer::bitcast(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr Pointer::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [&] (bool val) -> Domain::Ptr {
        llvm::APInt retval(1, 0, false);
        if (val) retval = 1;
        else retval = 0;
        return factory_->getInteger(factory_->toInteger(retval));
    };
    auto&& ptr = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            return (ptr->isNullptr()) ?
                   getBool(this->isNullptr()) :
                   factory_->getInteger(TOP, 1);

        case llvm::CmpInst::ICMP_NE:
            return (ptr->isNullptr()) ?
                   getBool(not this->isNullptr()) :
                   factory_->getInteger(TOP, 1);

        default:
            return factory_->getInteger(TOP, 1);
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

