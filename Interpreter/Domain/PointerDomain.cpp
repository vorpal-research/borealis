//
// Created by abdullin on 4/7/17.
//

#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/Domain/AggregateDomain.h"
#include "PointerDomain.h"
#include "Util/collections.hpp"
#include "Util/hash.hpp"
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

Domain::Ptr NullptrDomain::load(Type::Ptr type, Domain::Ptr) {
    errs() << "Load from nullptr" << endl;
    return factory_->getTop(type);
}

Domain::Ptr NullptrDomain::gep(Type::Ptr type, const std::vector<Domain::Ptr>&) {
    errs() << "GEP to nullptr" << endl;
    auto ptrType = factory_->getTypeFactory()->getPointer(type, 0);
    return factory_->getTop(ptrType);
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
PointerDomain::PointerDomain(Domain::Value value, DomainFactory* factory, Type::Ptr elementType, bool isGep)
        : Domain{value, POINTER, factory},
          elementType_(elementType),
          isGep_(isGep) {}

PointerDomain::PointerDomain(DomainFactory* factory, Type::Ptr elementType, const PointerDomain::Locations& locations, bool isGep)
        : Domain{VALUE, POINTER, factory},
          elementType_(elementType),
          locations_(locations),
          isGep_(isGep) {
    for (auto&& it : locations_) {
        if (auto&& st = llvm::dyn_cast<AggregateDomain>(it.location_.get())) {
            if (st->isStruct() && it.offsets_.begin()->get()->isTop()) {
                moveToTop();
                break;
            }
        }
    }
}

PointerDomain::PointerDomain(const PointerDomain& other)
        : Domain{other.value_, other.type_, other.factory_},
          elementType_(other.elementType_),
          locations_(other.locations_) {}

bool PointerDomain::equals(const Domain* other) const {
    auto ptr = llvm::dyn_cast<PointerDomain>(other);
    if (not ptr) return false;
    if (this == ptr) return true;
    if (this->value_ != ptr->value_) return false;

    if (locations_.size() != ptr->locations_.size()) return false;

    for (auto&& it : ptr->locations_) {
        auto&& its = locations_.find(it);
        if (its == locations_.end()) return false;
        if (not it.location_->equals(its->location_.get())) return false;
        if (not util::equal_with_find(it.offsets_, its->offsets_, LAM(a, a), LAM2(a, b, a->equals(b.get()))))
            return false;
    }
    return true;
}

bool PointerDomain::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

bool PointerDomain::isNullptr() const {
    for (auto&& it : locations_)
        if (it.location_->isNullptr()) return true;
    return false;
}

bool PointerDomain::onlyNullptr() const {
    return isNullptr() && locations_.size() == 1;
}

const PointerDomain::Locations& PointerDomain::getLocations() const {
    return locations_;
}

Type::Ptr PointerDomain::getElementType() const {
    return elementType_;
}

Domain::Ptr PointerDomain::clone() const {
    return std::make_shared<PointerDomain>(*this);
}

std::size_t PointerDomain::hashCode() const {
    return util::hash::simple_hash_value(value_, type_, locations_.size());
}

Domain::Ptr PointerDomain::getBound() const {
    auto type = factory_->getTypeFactory()->getInteger(DomainFactory::defaultSize);
    if (isTop()) return factory_->getTop(type);

    Domain::Ptr result = factory_->getBottom(type);
    for (auto&& it : locations_) {
        if (auto&& aggregate = llvm::dyn_cast<AggregateDomain>(it.location_.get())) {
            auto it_off = util::viewContainer(it.offsets_).reduce(factory_->getBottom(type), LAM2(acc, e, acc->join(e)));
            result = result->join(aggregate->getLength()->sub(it_off));
        }
    }
    return result;
}

std::string PointerDomain::toPrettyString(const std::string& prefix) const {
    std::ostringstream ss;
    ss << "Ptr " << TypeUtils::toString(*elementType_.get()) << " [";
    if (isTop()) ss << " TOP ]";
    else if (isBottom()) ss << " BOTTOM ]";
    else {
        for (auto&& it : locations_) {
            for (auto&& offset : it.offsets_) {
                ss << std::endl << prefix << "  " << offset->toString() << " " << it.location_->toPrettyString(prefix + "  ");
            }
        }
        ss << std::endl << prefix << "]";
    }
    return ss.str();
}

bool PointerDomain::classof(const Domain* other) {
    return other->getType() == Domain::POINTER;
}

Domain::Ptr PointerDomain::join(Domain::Ptr other) {
    if (this == other.get()) return shared_from_this();
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
                    ASSERT(it->offsets_.size() == 1 && itptr.offsets_.size() == 1, "unexpected")
                    Domain::Ptr curOffset = *it->offsets_.begin();
                    Domain::Ptr incOffset = *itptr.offsets_.begin();
                    it->offsets_ = {curOffset->join(incOffset)};
                } else {
                    for (auto&& offset : itptr.offsets_) it->offsets_.insert(offset);
                }
            } else {
                ASSERT(it->offsets_.size() == 1 && itptr.offsets_.size() == 1, "unexpected")
                Domain::Ptr curOffset = *it->offsets_.begin();
                Domain::Ptr incOffset = *itptr.offsets_.begin();
                it->offsets_ = {curOffset->join(incOffset)};
            }
        }
    }
    return shared_from_this();
}

Domain::Ptr PointerDomain::widen(Domain::Ptr other) {
    return join(other);
}

Domain::Ptr PointerDomain::meet(Domain::Ptr other) {
    if (this == other.get()) return shared_from_this();
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer meet");

    if (this == other.get()) return shared_from_this();

    if (other->isBottom()) {
        return other;
    } else if (this->isBottom()) {
        return shared_from_this();
    } else if (this->isTop()) {
        return shared_from_this();
    } else if (other->isTop()) {
        moveToTop();
        return shared_from_this();
    }

    for (auto&& it : locations_) {
        if (not util::contains(ptr->locations_, it)) {
            locations_.erase(it);
        }
    }

    for (auto&& itptr : ptr->locations_) {
        auto&& it = locations_.find(itptr);
        if (it == locations_.end()) {
            // take only similar locations
        } else {
            if (auto&& aggregate = llvm::dyn_cast<AggregateDomain>(itptr.location_.get())) {
                if (aggregate->isArray()) {
                    ASSERT(it->offsets_.size() == 1 && itptr.offsets_.size() == 1, "unexpected")
                    Domain::Ptr curOffset = *it->offsets_.begin();
                    Domain::Ptr incOffset = *itptr.offsets_.begin();
                    it->offsets_ = {curOffset->join(incOffset)};
                } else {
                    for (auto&& offset : itptr.offsets_) it->offsets_.insert(offset);
                }
            } else {
                ASSERT(it->offsets_.size() == 1 && itptr.offsets_.size() == 1, "unexpected")
                Domain::Ptr curOffset = *it->offsets_.begin();
                Domain::Ptr incOffset = *itptr.offsets_.begin();
                it->offsets_ = {curOffset->join(incOffset)};
            }
        }
    }
    return shared_from_this();
}

Domain::Ptr PointerDomain::sub(Domain::Ptr other) const {
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer sub");

    auto intTy = factory_->getTypeFactory()->getInteger(factory_->defaultSize);

    if (not (isValue() && other->isValue())) {
        return factory_->getTop(intTy);
    }

    if (locations_.size() != ptr->locations_.size()) factory_->getTop(intTy);

    auto result = factory_->getBottom(intTy);
    auto&& loc_end = locations_.end();
    for (auto&& it_ptr : ptr->locations_) {
        auto&& it = locations_.find(it_ptr);
        if (it == loc_end) return factory_->getTop(intTy);
        else {
            auto it_off = util::viewContainer(it->offsets_).reduce(factory_->getBottom(intTy), LAM2(acc, e, acc->join(e)));
            auto itptr_off = util::viewContainer(it_ptr.offsets_).reduce(factory_->getBottom(intTy), LAM2(acc, e, acc->join(e)));
            result = result->join(it_off->sub(itptr_off));
        }
    }

    return result;
}

Domain::Ptr PointerDomain::load(Type::Ptr type, Domain::Ptr offset) {
    if (isBottom()) {
        return factory_->getBottom(type);
    } else if (isTop()) {
        return factory_->getTop(type);
    }

    auto result = factory_->getBottom(type);
    for (auto&& it : locations_) {
        for (auto&& cur_offset : it.offsets_) {
            auto totalOffset = cur_offset->add(offset);
            result = result->join(it.location_->load(type, totalOffset));
        }
    }
    return result;
}

void PointerDomain::store(Domain::Ptr value, Domain::Ptr offset) {
    if (isTop()) return;

    if ((not isGep_) && llvm::isa<type::Pointer>(elementType_.get())) {
        locations_.clear();
        if (value->isAggregate()) {
            locations_.insert({ {factory_->getIndex(0)}, value});
        } else {
            auto arrayTy = factory_->getTypeFactory()->getArray(elementType_, 1);
            auto&& arrayDom = factory_->getAggregate(arrayTy, {value});
            locations_.insert({ {factory_->getIndex(0)}, arrayDom});
        }
        // if pointer was BOTTOM, then we should change it's value. But we can't create
        // new ptr domain, because there may be objects, referencing this one
        if (isBottom()) setValue();
    } else {
        for (auto&& it : locations_) {
            for (auto&& cur_offset : it.offsets_) {
                auto totalOffset = cur_offset->add(offset);
                it.location_->store(value, {totalOffset});
            }
        }
    }
}

Domain::Ptr PointerDomain::gep(Type::Ptr type, const std::vector<Domain::Ptr>& indices) {
    auto ptrType = factory_->getTypeFactory()->getPointer(type, 0);
    if (isBottom()) {
        return factory_->getBottom(ptrType);
    } else if (isTop()) {
        return factory_->getTop(ptrType);
    }

    auto result = factory_->getBottom(ptrType);
    std::vector<Domain::Ptr> subOffsets(indices.begin(), indices.end());
    auto zeroElement = subOffsets[0];

    for (auto&& it : locations_) {
        for (auto&& cur_offset : it.offsets_) {
            subOffsets[0] = zeroElement->add(cur_offset);
            result = result->join(it.location_->gep(type, subOffsets));
        }
    }
    return result;
}

Domain::Ptr PointerDomain::ptrtoint(Type::Ptr type) {
    moveToTop();
    return factory_->getTop(type);
}

Domain::Ptr PointerDomain::bitcast(Type::Ptr type) {
    moveToTop();
    return factory_->getTop(type);
}

Domain::Ptr PointerDomain::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    auto boolTy = factory_->getTypeFactory()->getBool();
    if (not (this->isValue() && other->isValue()))
        return factory_->getTop(boolTy);

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (not util::hasIntersection(locations_, ptr->locations_))
                return factory_->getBool(false);
            return this->equals(ptr) ?
                   factory_->getBool(true) :
                   factory_->getTop(boolTy);

        case llvm::CmpInst::ICMP_NE:
            if (not util::hasIntersection(locations_, ptr->locations_))
                return factory_->getBool(true);
            return this->equals(ptr) ?
                   factory_->getBool(false) :
                   factory_->getTop(boolTy);

        default:
            return factory_->getTop(boolTy);
    }
}

Split PointerDomain::splitByEq(Domain::Ptr other) {
    auto&& ptr = llvm::dyn_cast<PointerDomain>(other.get());
    ASSERT(ptr, "Non-pointer domain in pointer join");

    auto ptrType = factory_->getTypeFactory()->getPointer(elementType_, 0);
    if (this->onlyNullptr())
        return {shared_from_this(), shared_from_this()};
    if (this->isTop() && ptr->onlyNullptr()) {
        auto intTy = factory_->getTypeFactory()->getInteger(factory_->defaultSize);
        auto arrayTy = factory_->getTypeFactory()->getArray(elementType_);
        return {factory_->getNullptr(elementType_),
                factory_->getPointer(elementType_, {{{factory_->getTop(intTy)}, factory_->getTop(arrayTy)}})};
    }
    if (this->isTop())
        return {factory_->getTop(ptrType), factory_->getTop(ptrType)};
    if (this->isBottom())
        return {factory_->getBottom(ptrType), factory_->getBottom(ptrType)};
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
    if (isTop()) return;
    if (onlyNullptr()) return;
    setTop();
    for (auto&& it : locations_)
        it.location_->moveToTop();
    locations_.clear();
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

