//
// Created by abdullin on 4/7/17.
//

#include "DomainFactory.h"
#include "IntegerIntervalDomain.h"
#include "PointerDomain.h"
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
NullptrDomain::NullptrDomain(DomainFactory* factory)
        : Domain(VALUE, NULLPTR, factory) {}

bool NullptrDomain::equals(const Domain* other) const {
    return other->isNullptr();
}

bool NullptrDomain::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr NullptrDomain::join(Domain::Ptr) {
    return shared_from_this();
}

Domain::Ptr NullptrDomain::meet(Domain::Ptr) {
    return shared_from_this();
}

Domain::Ptr NullptrDomain::widen(Domain::Ptr) {
    return shared_from_this();
}

std::size_t NullptrDomain::hashCode() const {
    return 0;
}

std::string NullptrDomain::toPrettyString(const std::string&) const {
    return "nullptr";
}

void NullptrDomain::store(Domain::Ptr, Domain::Ptr) {
    errs() << "Store to nullptr" << endl;
}

Domain::Ptr NullptrDomain::load(const llvm::Type& type, Domain::Ptr) {
    errs() << "Load from nullptr" << endl;
    return factory_->getTop(type);
}

Domain::Ptr NullptrDomain::gep(const llvm::Type& type, const std::vector<Domain::Ptr>&) {
    errs() << "GEP to nullptr" << endl;
    return factory_->getPointer(TOP, type);
}

bool NullptrDomain::classof(const Domain* other) {
    return other->getType() == Domain::NULLPTR;
}

Domain::Ptr NullptrDomain::clone() const {
    return factory_->getNullptrLocation();
}

//////////////////////////////////////////////////////////
/// Pointer
//////////////////////////////////////////////////////////
PointerDomain::PointerDomain(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType)
        : Domain{value, POINTER, factory},
          elementType_(elementType) {}

PointerDomain::PointerDomain(DomainFactory* factory, const llvm::Type& elementType, const PointerDomain::Locations& locations)
        : Domain{VALUE, POINTER, factory},
          elementType_(elementType),
          locations_(locations) {}

PointerDomain::PointerDomain(const PointerDomain& other)
        : Domain{other.value_, other.type_, other.factory_},
          elementType_(other.elementType_),
          locations_(other.locations_) {}

bool PointerDomain::equals(const Domain* other) const {
    auto ptr = llvm::dyn_cast<PointerDomain>(other);
    if (not ptr) return false;
    if (this == ptr) return true;

    if (locations_.size() != ptr->locations_.size()) return false;

    return util::equal_with_find(locations_, ptr->locations_, LAM(a, a),
                                 LAM2(a, b, a.location_->equals(b.location_.get())));
}

bool PointerDomain::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

const PointerDomain::Locations& PointerDomain::getLocations() const {
    return locations_;
}

const llvm::Type& PointerDomain::getElementType() const {
    return elementType_;
}

Domain::Ptr PointerDomain::clone() const {
    return Domain::Ptr{ new PointerDomain(*this) };
}

std::size_t PointerDomain::hashCode() const {
    return util::hash::simple_hash_value(value_, type_, locations_.size());
}

std::string PointerDomain::toPrettyString(const std::string& prefix) const {
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

bool PointerDomain::classof(const Domain* other) {
    return other->getType() == Domain::POINTER;
}

Domain::Ptr PointerDomain::join(Domain::Ptr other) {
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    if (this == other.get()) return shared_from_this();

    if (other->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return other->shared_from_this();
    } else if (this->isTop()) {
        return shared_from_this();
    } else if (other->isTop()) {
        moveToTop();
        return shared_from_this();
    }

    for (auto&& itptr : ptr->locations_) {
        auto&& it = locations_.find(itptr);
        if (it == locations_.end()) {
            locations_.insert(itptr);
        /// Assume that length and location are same
        } else {
            if (auto&& aggregate = llvm::dyn_cast<AggregateDomain>(itptr.location_.get())) {
                if (aggregate->isArray()) {
                    it->offset_ = it->offset_->join(itptr.offset_);
                } else {
                    locations_.insert(itptr);
                }
            } else {
                it->offset_ = it->offset_->join(itptr.offset_);
            }
        }
    }
    return shared_from_this();
}

Domain::Ptr PointerDomain::widen(Domain::Ptr other) {
    return join(other);
}

Domain::Ptr PointerDomain::meet(Domain::Ptr) {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr PointerDomain::load(const llvm::Type& type, Domain::Ptr offset) {
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

void PointerDomain::store(Domain::Ptr value, Domain::Ptr offset) {
    if (isTop()) return;

    if (elementType_.isPointerTy()) {
        locations_.clear();
        if (value->isAggregate()) {
            locations_.insert({factory_->getIndex(0), value});
        } else {
            auto&& arrayType = llvm::ArrayType::get(const_cast<llvm::Type*>(&elementType_), 1);
            auto&& arrayDom = factory_->getAggregate(*arrayType, {value});
            locations_.insert({factory_->getIndex(0), arrayDom});
        }
        // if pointer was BOTTOM, then we should change it's value. But we can't create
        // new ptr domain, because there may be objects, referencing this one
        if (isBottom()) setValue();
    } else {
        for (auto&& it : locations_) {
            auto totalOffset = it.offset_->add(offset);
            it.location_->store(value, {totalOffset});
        }
    }
}

Domain::Ptr PointerDomain::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) {
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

Domain::Ptr PointerDomain::ptrtoint(const llvm::Type& type) {
    moveToTop();
    return factory_->getTop(type);
}

Domain::Ptr PointerDomain::bitcast(const llvm::Type& type) {
    if (elementType_.isFunctionTy() && type.isPointerTy() && type.getPointerElementType()->isFunctionTy()) {
        return factory_->getPointer(*type.getPointerElementType(), locations_);
    }
    moveToTop();
    return factory_->getTop(type);
}

Domain::Ptr PointerDomain::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    if (not (this->isValue() && other->isValue())) return factory_->getInteger(TOP, 1);

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (not util::hasIntersection(locations_, ptr->locations_))
                return factory_->getBool(false);
            return this->equals(ptr) ?
                   factory_->getBool(true) :
                   factory_->getInteger(TOP, 1);

        case llvm::CmpInst::ICMP_NE:
            if (not util::hasIntersection(locations_, ptr->locations_))
                return factory_->getBool(true);
            return this->equals(ptr) ?
                   factory_->getBool(false) :
                   factory_->getInteger(TOP, 1);

        default:
            return factory_->getInteger(TOP, 1);
    }
}

Split PointerDomain::splitByEq(Domain::Ptr other) {
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    if (this->isTop())
        return {factory_->getPointer(TOP, elementType_), factory_->getPointer(TOP, elementType_)};
    if (this->isBottom())
        return {factory_->getPointer(BOTTOM, elementType_), factory_->getPointer(BOTTOM, elementType_)};
    if (not other->isValue())
        return {shared_from_this(), shared_from_this()};

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

void PointerDomain::moveToTop() {
    setTop();
    for (auto&& it : locations_)
        it.location_->moveToTop();
    locations_.clear();
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

