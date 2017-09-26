//
// Created by abdullin on 4/7/17.
//

#include <unordered_set>

#include "AggregateDomain.h"
#include "DomainFactory.h"
#include "IntegerIntervalDomain.h"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

/// Struct constructors
AggregateDomain::AggregateDomain(DomainFactory* factory,
                                 const AggregateDomain::Types& elementTypes,
                                 Domain::Ptr length)
        : AggregateDomain(VALUE, factory, elementTypes, length) {}

AggregateDomain::AggregateDomain(Domain::Value value,
                                 DomainFactory* factory,
                                 const AggregateDomain::Types& elementTypes,
                                 Domain::Ptr length)
        : Domain{value, AGGREGATE, factory},
          aggregateType_(STRUCT),
          elementTypes_(elementTypes),
          length_(length) {
    if (isMaxLengthTop()) value_ = TOP;
}

AggregateDomain::AggregateDomain(DomainFactory* factory,
                                 const AggregateDomain::Types& elementTypes,
                                 const AggregateDomain::Elements& elements)
        : Domain{VALUE, AGGREGATE, factory},
          aggregateType_(STRUCT),
          elementTypes_(elementTypes),
          length_(factory_->getIndex(elements.size())),
          elements_(elements) {}

/// Array constructors
AggregateDomain::AggregateDomain(DomainFactory* factory,
                                 const llvm::Type& elementType,
                                 Domain::Ptr length)
        : AggregateDomain(VALUE, factory, elementType, length) {}

AggregateDomain::AggregateDomain(Domain::Value value,
                                 DomainFactory* factory,
                                 const llvm::Type& elementType,
                                 Domain::Ptr length)
        : Domain{value, AGGREGATE, factory},
          aggregateType_(ARRAY),
          elementTypes_({{0, &elementType}}),
          length_(length) {
    if (isMaxLengthTop()) value_ = TOP;
}

AggregateDomain::AggregateDomain(DomainFactory* factory,
                                 const llvm::Type& elementType,
                                 const AggregateDomain::Elements& elements)
        : Domain{VALUE, AGGREGATE, factory},
          aggregateType_(ARRAY),
          elementTypes_({{0, &elementType}}),
          length_(factory_->getIndex(elements.size())),
          elements_(elements) {}

AggregateDomain::AggregateDomain(const AggregateDomain& other)
        : Domain{other.value_, other.type_, other.factory_},
          aggregateType_(other.aggregateType_),
          elementTypes_(other.elementTypes_),
          length_(other.length_->clone()),
          elements_(other.elements_) {}

Domain::Ptr AggregateDomain::clone() const {
    return std::make_shared<AggregateDomain>(*this);
}

std::size_t AggregateDomain::hashCode() const {
    return util::hash::simple_hash_value(aggregateType_, type_, length_, elementTypes_.size());
}

std::string AggregateDomain::toPrettyString(const std::string& prefix) const {
    std::stringstream ss;

    if (isArray()) {
        ss << "Array [" << getMaxLength() << " x " << factory_->getSlotTracker().toString(&getElementType(0)) << "] ";
    } else if (isStruct()) {
        ss << "Struct {";
        for (auto&& it : elementTypes_) {
            ss << "  " << factory_->getSlotTracker().toString(it.second) << ", ";
        }
        ss << "} ";
    }
    ss << ": [";
    if (isTop()) {
        ss << " TOP ]";
    } else if (isBottom()) {
        ss << " BOTTOM ]";
    } else {
        for (auto&& it : elements_) {
            ss << std::endl << prefix << "  " << it.first << " : " << it.second->toPrettyString(prefix + "  ");
        }
        ss << std::endl << prefix << "]";
    }
    return ss.str();
}

const AggregateDomain::Types& AggregateDomain::getElementTypes() const {
    return elementTypes_;
}

const AggregateDomain::Elements& AggregateDomain::getElements() const {
    return elements_;
}

Domain::Ptr AggregateDomain::getLength() const {
    return length_;
}

std::size_t AggregateDomain::getMaxLength() const {
    auto intLength = llvm::cast<IntegerIntervalDomain>(length_.get());
    return intLength->ub()->getRawValue();
}

bool AggregateDomain::isMaxLengthTop() const {
    auto intLength = llvm::cast<IntegerIntervalDomain>(length_.get());
    return intLength->ub()->isMax();
}

bool AggregateDomain::equals(const Domain* other) const {
    auto aggregate = llvm::dyn_cast<AggregateDomain>(other);
    if (not aggregate) return false;
    if (this == aggregate) return true;

    if (aggregateType_ != aggregate->aggregateType_) return false;
    if (elementTypes_.size() != aggregate->elementTypes_.size()) return false;
    if (not getLength()->equals(aggregate->getLength().get())) return false;
    if (elementTypes_ != aggregate->elementTypes_) return false;

    for (auto&& it : elements_) {
        auto&& opt = util::at(aggregate->elements_, it.first);
        if ((not opt) || (not it.second->equals(opt.getUnsafe().get()))) {
            return false;
        }
    }

    return true;
}

bool AggregateDomain::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

bool AggregateDomain::classof(const Domain* other) {
    return other->getType() == Domain::AGGREGATE;
}

Domain::Ptr AggregateDomain::join(Domain::Ptr other) {
    if (this == other.get()) return shared_from_this();
    auto&& aggregate = llvm::dyn_cast<AggregateDomain>(other.get());
    ASSERT(aggregate, "Non-aggregate int join");

    if (this->isBottom())
        return other;
    else if (other->isBottom())
        return shared_from_this();

    length_ = length_->join(aggregate->length_);
    for (auto&& it : aggregate->getElements()) {
        if (util::at(getElements(), it.first)) {
            elements_[it.first] = elements_[it.first]->join(it.second);
        } else {
            elements_[it.first] = it.second;
        }
    }

    return shared_from_this();
}

Domain::Ptr AggregateDomain::widen(Domain::Ptr other) {
    auto&& aggregate = llvm::dyn_cast<AggregateDomain>(other.get());
    ASSERT(aggregate, "Widening aggregate with non-aggregate");

    if (this->isBottom())
        return other;
    else if (other->isBottom())
        return shared_from_this();

    length_ = length_->join(aggregate->length_);
    for (auto&& it : aggregate->getElements()) {
        if (util::at(getElements(), it.first)) {
            elements_[it.first] = elements_[it.first]->widen(it.second);
        } else {
            elements_[it.first] = it.second;
        }
    }

    return shared_from_this();
}

Domain::Ptr AggregateDomain::meet(Domain::Ptr) {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr AggregateDomain::extractValue(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) {
    auto length = getMaxLength();
    auto idx_interval = llvm::dyn_cast<IntegerIntervalDomain>(indices.begin()->get());
    ASSERT(idx_interval, "Unknown type of offsets");

    if (isBottom()) {
        return factory_->getBottom(type);
    } else if (isTop()) {
        return factory_->getTop(type);
    }

    auto idx_begin = idx_interval->lb()->getRawValue();
    auto idx_end = idx_interval->ub()->getRawValue();

    if (idx_end > length) {
        warns() << "Possible buffer overflow" << endl;
    } else if (idx_begin > length) {
        warns() << "Buffer overflow" << endl;
    }

    Domain::Ptr result = factory_->getBottom(type);
    std::vector<Domain::Ptr> sub_idx(indices.begin() + 1, indices.end());
    for (auto i = idx_begin; i <= idx_end && i < length; ++i) {
        if (not util::at(elements_, i)) {
            elements_[i] = factory_->getBottom(getElementType(i));
        }
        result = indices.size() == 1 ?
                 result->join(elements_.at(i)) :
                 result->join(elements_.at(i)->extractValue(type, sub_idx));
    }
    return result;
}

void AggregateDomain::insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) {
    auto length = getMaxLength();
    auto idx_interval = llvm::dyn_cast<IntegerIntervalDomain>(indices.begin()->get());
    ASSERT(idx_interval, "Unknown type of offsets");

    if (not isValue())
        return;

    auto idx_begin = idx_interval->lb()->getRawValue();
    auto idx_end = idx_interval->ub()->getRawValue();

    if (idx_end > length) {
        warns() << "Possible buffer overflow" << endl;
    } else if (idx_begin > length) {
        warns() << "Buffer overflow" << endl;
    }

    std::vector<Domain::Ptr> sub_idx(indices.begin() + 1, indices.end());
    for (auto i = idx_begin; i <= idx_end && i < length; ++i) {
        if (not util::at(elements_, i)) {
            elements_[i] = factory_->getBottom(getElementType(i));
        }
        if (indices.size() == 1) {
            elements_[i] = elements_[i]->join(element);
        } else {
            elements_[i]->insertValue(element, sub_idx);
        }
    }
}

Domain::Ptr AggregateDomain::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) {
    auto length = getMaxLength();
    auto idx_interval = llvm::dyn_cast<IntegerIntervalDomain>(indices.begin()->get());
    ASSERT(idx_interval, "Unknown type of offsets");

    auto ptrType = llvm::PointerType::get(const_cast<llvm::Type*>(&type), 0);
    if (isBottom()) {
        return factory_->getBottom(*ptrType);
    } else if (isTop()) {
        return factory_->getTop(*ptrType);
    }

    auto idx_begin = idx_interval->lb()->getRawValue();
    auto idx_end = idx_interval->ub()->getRawValue();

    if (idx_end > length) {
        warns() << "Possible buffer overflow" << endl;
    } else if (idx_begin > length) {
        warns() << "Buffer overflow" << endl;
    }

    if (indices.size() == 1) {
        return factory_->getPointer(type, { { {indices[0]}, shared_from_this()} });

    } else {
        Domain::Ptr result = nullptr;

        std::vector<Domain::Ptr> sub_idx(indices.begin() + 1, indices.end());
        for (auto i = idx_begin; i <= idx_end && i < length; ++i) {
            if (not util::at(elements_, i)) {
                elements_[i] = factory_->getBottom(getElementType(i));
            }
            auto subGep = elements_[i]->gep(type, sub_idx);
            result = result ?
                     result->join(subGep) :
                     subGep;
        }
        if (not result) {
            warns() << "Gep is out of bounds" << endl;
            return factory_->getTop(*ptrType);
        }

        return result;
    }
}

void AggregateDomain::store(Domain::Ptr value, Domain::Ptr offset) {
    return insertValue(value, {offset});
}

Domain::Ptr AggregateDomain::load(const llvm::Type& type, Domain::Ptr offset) {
    return extractValue(type, {offset});
}

const llvm::Type& AggregateDomain::getElementType(std::size_t index) const {
    return *(isArray() ? elementTypes_.at(0) : elementTypes_.at(index));
}

bool AggregateDomain::isArray() const {
    return aggregateType_ == ARRAY;
}

bool AggregateDomain::isStruct() const {
    return aggregateType_ == STRUCT;
}

void AggregateDomain::moveToTop() {
    setTop();
    for (auto&& it : elements_) {
        auto&& content = it.second;
        if (content->isMutable()) content->moveToTop();
    }
    elements_.clear();
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"