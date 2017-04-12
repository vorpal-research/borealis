//
// Created by abdullin on 4/7/17.
//

#include "Array.h"
#include "DomainFactory.h"
#include "IntegerInterval.h"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Array::Array(Domain::Value value, DomainFactory* factory, const llvm::Type& elementType)
        : Domain{value, Domain::ARRAY, factory},
          elementType_(elementType) {}

Array::Array(DomainFactory* factory, const llvm::Type& elementType, Domain::Ptr length)
        : Domain{VALUE, Domain::ARRAY, factory},
          elementType_(elementType),
          length_(length) {
    auto&& intLen = llvm::dyn_cast<IntegerInterval>(length_.get());
    ASSERT(intLen, "Non-integer length in array");

    auto maxLen = *intLen->to().getRawData();
    for (auto i = 0U; i < maxLen; ++i)
        elements_.push_back(factory_->getBottom(elementType));
}

Array::Array(DomainFactory* factory, const llvm::Type& elementType, const Array::Elements& elements)
        : Domain{VALUE, Domain::ARRAY, factory},
          elementType_(elementType),
          elements_(elements) {
    length_ = factory_->getIndex(elements_.size());
}

Array::Array(const Array& other)
        : Domain{other.value_, other.type_, other.factory_},
          elementType_(other.elementType_),
          elements_(other.elements_),
          length_(other.length_) {}

Domain& Array::operator=(const Domain& other) {
    auto&& array = llvm::dyn_cast<Array>(&other);
    ASSERT(array, "Nullptr in array join");
    if (this == array) return *this;

    Domain::operator=(other);
    ASSERT(elementType_.getTypeID() == array->elementType_.getTypeID(), "Different array types in assigment");
    elements_ = array->elements_;
    length_ = array->length_;
    return *this;
}

bool Array::equals(const Domain* other) const {
    auto&& array = llvm::dyn_cast<Array>(other);
    if (not array) return false;
    if (this == other) return true;

    if (this->isBottom() && other->isBottom()) return true;
    if (this->isTop() && other->isTop()) return true;

    return elementType_.getTypeID() == array->elementType_.getTypeID() &&
            length_->equals(array) &&
            elements_ == array->elements_;
}

bool Array::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

size_t Array::hashCode() const {
    return util::hash::simple_hash_value(value_, length_, elements_);
}

std::string Array::toString() const {
    std::ostringstream ss;
    if (isBottom())
        ss << "[ BOTTOM ]";
    else if (isTop())
        ss << "[ TOP ]";
    else {
        ss << "[";
        for (auto i = 0U; i < elements_.size(); ++i) {
            ss << elements_[i]->toString();
            if (i != elements_.size() - 1) ss << ", ";
        }
        ss << "]";
    }
    return ss.str();
}

bool Array::classof(const Domain* other) {
    return other->getType() == Domain::ARRAY;
}

Domain* Array::clone() const {
    return new Array(*this);
}

const Array::Elements& Array::getElements() const {
    return elements_;
}

Domain::Ptr Array::getLength() const {
    return length_;
}

size_t Array::getMaxLength() const {
    auto&& intLen = llvm::dyn_cast<IntegerInterval>(length_.get());
    ASSERT(intLen, "Non-integer length in array");

    return *intLen->to().getRawData();
}

Domain::Ptr Array::join(Domain::Ptr other) const {
    auto&& array = llvm::dyn_cast<Array>(other.get());
    ASSERT(array, "Joining array with non-array");

    length_ = length_->join(array->length_);
    for (auto i = 0U; i < elements_.size(); ++i) {
        if (i < array->getMaxLength())
            elements_[i] = elements_[i]->join(array->elements_[i]);
    }

    return shared_from_this();
}

Domain::Ptr Array::widen(Domain::Ptr other) const {
    auto&& array = llvm::dyn_cast<Array>(other.get());
    ASSERT(array, "Joining array with non-array");

    length_ = length_->join(array->length_);
    for (auto i = 0U; i < elements_.size(); ++i) {
        if (i < array->getMaxLength())
            elements_[i] = elements_[i]->widen(array->elements_[i]);
    }

    return shared_from_this();
}

Domain::Ptr Array::meet(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Array::narrow(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr Array::extractValue(const std::vector<Domain::Ptr>& indices) const {
    auto maxLength = getMaxLength();
    auto indexInterval = llvm::dyn_cast<IntegerInterval>(indices.begin()->get());
    ASSERT(indexInterval, "Unknown type of offsets");

    auto indexStart = *indexInterval->from().getRawData();
    auto indexEnd = *indexInterval->from().getRawData();

    if (indexEnd > maxLength) {
        warns() << "Possible buffer overflow" << endl;
    } else if (indexStart > maxLength) {
        warns() << "Buffer overflow" << endl;
    }

    if (isBottom())
        return factory_->getPointer(elementType_, { factory_->getBottom(elementType_) });
    else if (isTop())
        return factory_->getPointer(elementType_, { factory_->getTop(elementType_) });

    Domain::Ptr result = factory_->getBottom(elementType_);
    if (indices.size() == 1) {
        for (auto i = indexStart; i <indexEnd && i < maxLength; ++i)
            result = result->join(elements_[i]);

    } else {
        UNREACHABLE("Unimplemented, sorry...");
    }
    return result;
}

void Array::insertValue(Domain::Ptr element, const std::vector<Domain::Ptr>& indices) const {
    auto maxLength = getMaxLength();
    auto indexInterval = llvm::dyn_cast<IntegerInterval>(indices.begin()->get());
    ASSERT(indexInterval, "Unknown type of offsets");

    auto indexStart = *indexInterval->from().getRawData();
    auto indexEnd = *indexInterval->from().getRawData();

    if (indexEnd > maxLength) {
        warns() << "Possible buffer overflow" << endl;
    } else if (indexStart > maxLength) {
        warns() << "Buffer overflow" << endl;
    }

    if (indices.size() == 1) {
        for (auto i = indexStart; i <indexEnd && i < maxLength; ++i)
            elements_[i] = elements_[i]->join(element);

    } else {
        UNREACHABLE("Unimplemented, sorry...");
    }
}

Domain::Ptr Array::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const {
    auto maxLength = getMaxLength();
    auto indexInterval = llvm::dyn_cast<IntegerInterval>(indices.begin()->get());
    ASSERT(indexInterval, "Unknown type of offsets");

    if (isBottom())
        return factory_->getPointer(type, { factory_->getBottom(type) });
    else if (isTop())
        return factory_->getPointer(type, { factory_->getTop(type) });

    auto indexStart = *indexInterval->from().getRawData();
    auto indexEnd = *indexInterval->from().getRawData();

    if (indexEnd > maxLength) {
        warns() << "Possible buffer overflow" << endl;
    } else if (indexStart > maxLength) {
        warns() << "Buffer overflow" << endl;
    }

    std::vector<Domain::Ptr> locations;
    if (indices.size() == 1) {
        for (auto i = indexStart; i <= indexEnd && i < maxLength; ++i) {
            locations.push_back(elements_[i]);
        }

    } else {
        std::vector<Domain::Ptr> subIndices(indices.begin() + 1, indices.end());
        for (auto i = indexStart; i <= indexEnd && i < maxLength; ++i)
            locations.push_back(elements_[i]->gep(type, subIndices));
    }
    return factory_->getPointer(type, locations);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"