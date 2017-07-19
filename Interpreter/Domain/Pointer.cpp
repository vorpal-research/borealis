//
// Created by abdullin on 4/7/17.
//

#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Pointer.h"
#include "Interpreter/Util.hpp"
#include "Util/collections.hpp"
#include "Util/hash.hpp"
#include "Util/streams.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

//////////////////////////////////////////////////////////
/// Nullpointer
//////////////////////////////////////////////////////////
Nullptr::Nullptr(DomainFactory* factory)
        : Domain(VALUE, NULLPTR, factory) {}

bool Nullptr::equals(const Domain* other) const {
    return llvm::isa<Nullptr>(other);
}

bool Nullptr::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Nullptr::join(Domain::Ptr) const {
    return shared_from_this();
}

Domain::Ptr Nullptr::meet(Domain::Ptr) const {
    return shared_from_this();
}

Domain::Ptr Nullptr::widen(Domain::Ptr) const {
    return shared_from_this();
}

std::size_t Nullptr::hashCode() const {
    return 0;
}

std::string Nullptr::toPrettyString(const std::string&) const {
    return "nullptr";
}

void Nullptr::store(Domain::Ptr, Domain::Ptr) const {
    errs() << "Store to nullptr" << endl;
}

Domain::Ptr Nullptr::load(const llvm::Type& type, Domain::Ptr) const {
    errs() << "Load from nullptr" << endl;
    return factory_->getTop(type);
}

Domain::Ptr Nullptr::gep(const llvm::Type& type, const std::vector<Domain::Ptr>&) const {
    errs() << "GEP to nullptr" << endl;
    return factory_->getPointer(TOP, type);
}

bool Nullptr::classof(const Domain* other) {
    return other->getType() == Domain::NULLPTR;
}

//////////////////////////////////////////////////////////
/// Pointer
//////////////////////////////////////////////////////////
Pointer::Pointer(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType)
        : Domain{value, POINTER, factory},
          elementType_(elementType) {}

Pointer::Pointer(DomainFactory* factory, const llvm::Type& elementType, const Pointer::Locations& locations)
        : Domain{VALUE, POINTER, factory},
          elementType_(elementType),
          locations_(locations) {}

bool Pointer::equals(const Domain* other) const {
    auto ptr = llvm::dyn_cast<Pointer>(other);
    if (not ptr) return false;
    if (this == ptr) return true;

    if (locations_.size() != ptr->locations_.size()) return false;

    return util::equal_with_find(locations_, ptr->locations_,
                                 [](auto&& a) { return a; },
                                 [](auto&& a, auto&& b) { return a.location_->equals(b.location_.get()); });
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
    return util::hash::simple_hash_value(value_, type_, locations_.size());
}

std::string Pointer::toPrettyString(const std::string& prefix) const {
    std::ostringstream ss;
    ss << "Ptr " << factory_->getSlotTracker().toString(&elementType_) << " [";
    if (isTop()) ss << " TOP ]";
    else if (isBottom()) ss << " BOTTOM ]";
    else {
        for (auto&& it : locations_) {
            ss << std::endl << prefix << "  " << it.offset_->toString() << " " << it.location_->toPrettyString(prefix + "  ");
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
        /// Assume that length and location are same
        } else {
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
    if (isTop()) return;

    if (elementType_.isPointerTy()) {
        locations_.clear();
        locations_.insert({factory_->getIndex(0), value});
        // if pointer was BOTTOM, then we should change it's value. But we can't create
        // new ptr domain, because there may be objects, referencing this one
        if (isBottom()) {
            // This is generally fucked up
            auto val = const_cast<Value*>(&value_);
            *val = VALUE;
        }
    } else {
        for (auto&& it : locations_) {
            auto totalOffset = it.offset_->add(offset);
            it.location_->store(value, {totalOffset});
        }
    }
}

Domain::Ptr Pointer::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const {
    if (isBottom()) {
        return factory_->getPointer(BOTTOM, type);
    } else if (isTop()) {
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
    if (elementType_.isFunctionTy() && type.isPointerTy() && type.getPointerElementType()->isFunctionTy()) {
        return factory_->getPointer(*type.getPointerElementType(), locations_);
    }
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

    if (this->isTop() || other->isTop()) return factory_->getInteger(TOP, 1);

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (not util::hasIntersection(locations_, ptr->locations_))
                return getBool(false);
            return this->equals(ptr) ?
                   getBool(true) :
                   factory_->getInteger(TOP, 1);

        case llvm::CmpInst::ICMP_NE:
            if (not util::hasIntersection(locations_, ptr->locations_))
                return getBool(true);
            return this->equals(ptr) ?
                   getBool(false) :
                   factory_->getInteger(TOP, 1);

        default:
            return factory_->getInteger(TOP, 1);
    }
}

Split Pointer::splitByEq(Domain::Ptr other) const {
    auto&& ptr = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    if (this->isTop() || other->isTop())
        return {factory_->getPointer(TOP, elementType_), factory_->getPointer(TOP, elementType_)};

    Locations trueLocs, falseLocs;
    for (auto&& loc : ptr->getLocations()) {
        if (util::contains(locations_, loc)) {
            trueLocs.insert(loc);
        } else {
            falseLocs.insert(loc);
        }
    }
    return {factory_->getPointer(elementType_, trueLocs),
            factory_->getPointer(elementType_, falseLocs)};
}

void Pointer::moveToTop() const {
    auto val = const_cast<Domain::Value*>(&value_);
    *val = TOP;
    for (auto&& it : locations_)
        it.location_->moveToTop();
    locations_.clear();
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

