//
// Created by abdullin on 4/7/17.
//

#include <unordered_set>

#include "AggregateObject.h"
#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

/// Struct constructors
AggregateObject::AggregateObject(DomainFactory* factory,
                                 const AggregateObject::Types& elementTypes,
                                 Domain::Ptr length)
        : AggregateObject(VALUE, factory, elementTypes, length) {}

AggregateObject::AggregateObject(Domain::Value value,
                                 DomainFactory* factory,
                                 const AggregateObject::Types& elementTypes,
                                 Domain::Ptr length)
        : Domain{value, AGGREGATE, factory},
          aggregateType_(STRUCT),
          elementTypes_(elementTypes),
          length_(length) {}

AggregateObject::AggregateObject(DomainFactory* factory,
                                 const AggregateObject::Types& elementTypes,
                                 const AggregateObject::Elements& elements)
        : Domain{VALUE, AGGREGATE, factory},
          aggregateType_(STRUCT),
          elementTypes_(elementTypes),
          length_(factory_->getIndex(elements.size())),
          elements_(elements) {}

/// Array constructors
AggregateObject::AggregateObject(DomainFactory* factory,
                                 const llvm::Type& elementType,
                                 Domain::Ptr length)
        : AggregateObject(VALUE, factory, elementType, length) {}

AggregateObject::AggregateObject(Domain::Value value,
                                 DomainFactory* factory,
                                 const llvm::Type& elementType,
                                 Domain::Ptr length)
        : Domain{value, AGGREGATE, factory},
          aggregateType_(ARRAY),
          elementTypes_({{0, &elementType}}),
          length_(length) {}

AggregateObject::AggregateObject(DomainFactory* factory,
                                 const llvm::Type& elementType,
                                 const AggregateObject::Elements& elements)
        : Domain{VALUE, AGGREGATE, factory},
          aggregateType_(ARRAY),
          elementTypes_({{0, &elementType}}),
          length_(factory_->getIndex(elements.size())),
          elements_(elements) {}

AggregateObject::AggregateObject(const AggregateObject& other)
        : Domain{VALUE, AGGREGATE, other.factory_},
          aggregateType_(other.aggregateType_),
          elementTypes_(other.elementTypes_),
          length_(other.length_),
          elements_(other.elements_) {}


std::size_t AggregateObject::hashCode() const {
    // TODO: proper hash
    return 0;//util::hash::simple_hash_value(aggregateType_, type_, length_, elementTypes_, elements_);
}

std::string AggregateObject::toString() const {
    std::stringstream ss;

    if (isArray()) ss << "Array " << util::toString(getTypeFor(0));
    else {
        ss << "Struct {";
        for (auto&& it : elementTypes_) {
            ss << util::toString(*it.second) << ", ";
        }
        ss << "}";
    }
    ss << ": [";
    for (auto&& it : elements_) {
        ss << std::endl << it.first << " : " << it.second->toString();
    }
    ss << std::endl << "]";
    return ss.str();
}

Domain* AggregateObject::clone() const {
    return new AggregateObject(*this);
}

const AggregateObject::Types& AggregateObject::getElementTypes() const {
    return elementTypes_;
}

const AggregateObject::Elements& AggregateObject::getElements() const {
    return elements_;
}

Domain::Ptr AggregateObject::getLength() const {
    return length_;
}

std::size_t AggregateObject::getMaxLength() const {
    auto intLength = llvm::cast<IntegerInterval>(length_.get());
    return *intLength->to().getRawData();
}

bool AggregateObject::equals(const Domain* other) const {
    auto aggregate = llvm::dyn_cast<AggregateObject>(other);
    if (not aggregate) return false;
    if (this == aggregate) return true;

    if (aggregateType_ != aggregate->aggregateType_) return false;

    for (auto&& it : elementTypes_) {
        if (auto&& opt = util::at(aggregate->elementTypes_, it.first)) {
            if (not (it.second->getTypeID() == opt.getUnsafe()->getTypeID()))
                return false;
        }
    }

    for (auto&& it : elements_) {
        if (auto&& opt = util::at(aggregate->elements_, it.first)) {
            if (not it.second->equals(opt.getUnsafe().get()))
                return false;
        }
    }

    return  getLength()->equals(aggregate->getLength().get());
}

bool AggregateObject::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

bool AggregateObject::classof(const Domain* other) {
    return other->getType() == Domain::AGGREGATE;
}

Domain::Ptr AggregateObject::join(Domain::Ptr other) const {
    auto&& aggregate = llvm::dyn_cast<AggregateObject>(other.get());
    ASSERT(aggregate, "Non-aggregate int join");

    if (this->isBottom())
        return other;
    else if (other->isBottom())
        return shared_from_this();

    length_ = length_->join(aggregate->length_);
    for (auto&& it : aggregate->getElements()) {
        if (util::at(getElements(), it.first)) {
            elements_[it.first]->store(it.second->load());
        } else {
            elements_[it.first] = it.second;
        }
    }

    return shared_from_this();
}

Domain::Ptr AggregateObject::widen(Domain::Ptr other) const {
    auto&& aggregate = llvm::dyn_cast<AggregateObject>(other.get());
    ASSERT(aggregate, "Widening aggregate with non-aggregate");

    if (this->isBottom())
        return other;
    else if (other->isBottom())
        return shared_from_this();

    length_ = length_->join(aggregate->length_);
    for (auto&& it : aggregate->getElements()) {
        if (util::at(getElements(), it.first)) {
            auto content = elements_[it.first]->load();
            elements_[it.first]->store(content->widen(it.second->load()));
        } else {
            elements_[it.first] = it.second;
        }
    }

    return shared_from_this();
}

Domain::Ptr AggregateObject::meet(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr AggregateObject::narrow(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr AggregateObject::extractValue(const llvm::Type& type, Domain::Ptr index) const {
    auto maxLength = getMaxLength();
    auto indexInterval = llvm::dyn_cast<IntegerInterval>(index.get());
    ASSERT(indexInterval, "Unknown type of offsets");

    if (isBottom()) {
        return factory_->getBottom(type);
    } else if (isTop()) {
        return factory_->getTop(type);
    }

    auto indexStart = *indexInterval->from().getRawData();
    auto indexEnd = *indexInterval->from().getRawData();

    if (indexEnd > maxLength) {
        warns() << "Possible buffer overflow" << endl;
    } else if (indexStart > maxLength) {
        warns() << "Buffer overflow" << endl;
    }

    Domain::Ptr result = factory_->getBottom(type);
    for (auto i = indexStart; i <= indexEnd && i < maxLength; ++i) {
        ASSERT(getTypeFor(i).getTypeID() == type.getTypeID(), "Wrong types in aggregate extractValue");
        if (not util::at(elements_, i)) {
            elements_[i] = factory_->getMemoryObject(getTypeFor(i));
        }
        result = result->join(elements_.at(i)->load());
    }
    return result;
}

void AggregateObject::insertValue(Domain::Ptr element, Domain::Ptr index) const {
    auto maxLength = getMaxLength();
    auto indexInterval = llvm::dyn_cast<IntegerInterval>(index.get());
    ASSERT(indexInterval, "Unknown type of offsets");

    if (isBottom() || isTop())
        return;

    auto indexStart = *indexInterval->from().getRawData();
    auto indexEnd = *indexInterval->from().getRawData();

    if (indexEnd > maxLength) {
        warns() << "Possible buffer overflow" << endl;
    } else if (indexStart > maxLength) {
        warns() << "Buffer overflow" << endl;
    }

    for (auto i = indexStart; i <= indexEnd && i < maxLength; ++i) {
        if (auto&& opt = util::at(elements_, i)) {
            opt.getUnsafe()->store(element);
        } else {
            elements_[i] = factory_->getMemoryObject(getTypeFor(i));
            elements_[i]->store(element);
        }
    }
}

Domain::Ptr AggregateObject::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const {
    auto maxLength = getMaxLength();
    auto indexInterval = llvm::dyn_cast<IntegerInterval>(indices.begin()->get());
    ASSERT(indexInterval, "Unknown type of offsets");

    if (isBottom())
        return factory_->getBottom(type);
    else if (isTop())
        return factory_->getTop(type);

    auto indexStart = *indexInterval->from().getRawData();
    auto indexEnd = *indexInterval->from().getRawData();

    if (indexEnd > maxLength) {
        warns() << "Possible buffer overflow" << endl;
    } else if (indexStart > maxLength) {
        warns() << "Buffer overflow" << endl;
    }

    if (indices.size() == 1) {
        llvm::APInt start(64, indexStart);
        llvm::APInt end(64, indexEnd);
        return factory_->getPointer(type, { {factory_->getInteger(start, end), shared_from_this()} });

    } else {
//        Domain::Ptr result = nullptr;
//
//        std::vector<Domain::Ptr> subIndices(indices.begin() + 1, indices.end());
//        for (auto i = indexStart; i <= indexEnd && i < maxLength; ++i) {
//            if (not util::at(elements_, i)) {
//                elements_[i] = factory_->getMemoryObject(getTypeFor(i));
//            }
//            auto subGep = elements_[i]->load()->gep(type, subIndices);
//            result = result ?
//                     result->join(subGep) :
//                     subGep;
//        }
//
//        return result;
    }
    ASSERT(false, "error");
    return nullptr;
}

void AggregateObject::store(Domain::Ptr value, Domain::Ptr offset) const {
    return insertValue(value, offset);
}

Domain::Ptr AggregateObject::load(const llvm::Type& type, Domain::Ptr offset) const {
    return extractValue(type, offset);
}

const llvm::Type& AggregateObject::getTypeFor(std::size_t index) const {
    if (isArray())
        return *elementTypes_.at(0);
    else
        return *elementTypes_.at(index);
}

bool AggregateObject::isArray() const {
    return aggregateType_ == ARRAY;
}

bool AggregateObject::isStruct() const {
    return aggregateType_ == STRUCT;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"