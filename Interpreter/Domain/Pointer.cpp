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

bool Pointer::equals(const Domain* other) const {
    auto ptr = llvm::dyn_cast<Pointer>(other);
    if (not ptr) return false;
    if (this == ptr) return true;

    if (locations_.size() != ptr->locations_.size()) return false;

    for (auto i = 0U; i < locations_.size(); ++i) {
        if (not locations_[i]->equals(ptr->locations_[i].get())) return false;
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
    return util::hash::simple_hash_value(value_, getType(), getElementType().getTypeID(), getLocations());
}

std::string Pointer::toString() const {
    std::ostringstream ss;
    ss << "Ptr " << util::toString(elementType_) << " [";
    if (isTop()) ss << " TOP ]" << std::endl;
    else if (isBottom()) ss << " BOTTOM ]" << std::endl;
    else {
        for (auto&& it : locations_) {
            ss << std::endl << it->toString();
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
    if (other->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return other->shared_from_this();
    } else if (other->isTop() || this->isTop()) {
        return factory_->getPointer(TOP, elementType_);
    } else if (auto&& ptr = llvm::dyn_cast<Pointer>(other.get())) {
        for (auto&& it : locations_) {
            for (auto&& itptr : ptr->locations_) {
                it = it->join(itptr);
            }
        }
        return shared_from_this();

    } else if (auto&& gep = llvm::dyn_cast<GepPointer>(other.get())) {
        ASSERT(elementType_.getTypeID() == gep->getElementType().getTypeID(), "Different types in ptr and gep join");

        if (elementType_.isPointerTy() || elementType_.isAggregateType()) {
            for (auto&& it : locations_) {
                for (auto&& mo : gep->getObjects()) {
                    it = it->join(mo->load());
                }
            }

        } else {
            for (auto&& it : locations_) {
                for (auto&& mo : gep->getObjects()) {
                    it->insertValue(mo->load(), {factory_->getIndex(0)});
                }
            }
        }
        return shared_from_this();
    }
    UNREACHABLE("Unknown domain in pointer join");
}

Domain::Ptr Pointer::widen(Domain::Ptr other) const {
    auto&& ptr = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(ptr, "Non-pointer in pointer domain");
    for (auto&& it : locations_) {
        for (auto&& itptr : ptr->locations_) {
            it = it->widen(itptr);
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
    if (isBottom()) {
        return factory_->getBottom(type);
    } else if (isTop()) {
        return factory_->getTop(type);
    }

    auto result = factory_->getBottom(elementType_);
    if (elementType_.isAggregateType()) {
        if (offsets.size() == 1) {
            auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
            ASSERT(intOffset->isConstant(0), "Big offset in integer");

            ASSERT(elementType_.getTypeID() == type.getTypeID(), "Different load types");
            for (auto&& it : locations_)
                result = result->join(it);

        } else {
            std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
            for (auto&& it : locations_)
                result = result->join(it->extractValue(type, subOffsets));
        }

    } else if (elementType_.isPointerTy()) {
        if (offsets.size() == 1) {
            auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
            ASSERT(elementType_.getTypeID() == type.getTypeID(), "Different load types");
            ASSERT(intOffset->isConstant(0), "Big offset in integer");
            for (auto&& it : locations_) {
                result = result->join(it);
            }

        } else {
            std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
            for (auto&& it : locations_)
                result = result->join(it->load(type, subOffsets));
        }

    } else {
        for (auto&& it : locations_) {
            result = result->join(it->extractValue(type, offsets));
        }
    }
    return result;
}

void Pointer::store(Domain::Ptr value, const std::vector<Domain::Ptr>& offsets) const {
    if (elementType_.isAggregateType()) {
        UNREACHABLE("Don't know what to do");
    } else if (elementType_.isPointerTy()) {
        if (offsets.size() == 1) {
            auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
            ASSERT(intOffset->isConstant(0), "Big offset in integer");
            for (auto&& it : locations_) {
                it = it->join(value);
            }

        } else {
            std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
            for (auto&& it : locations_) {
                it->store(value, subOffsets);
            }
        }

    } else {
        ASSERT(offsets.size() == 1, "Too big offset in pointer");
        for (auto&& it : locations_)
            it->insertValue(value, offsets);
    }
}

Domain::Ptr Pointer::gep(const llvm::Type& type, const std::vector<Domain::Ptr>& indices) const {
    if (isBottom())
        return factory_->getPointer(BOTTOM, type);
    else if (isTop())
        return factory_->getPointer(TOP, type);

    Domain::Ptr result = nullptr;
    if (elementType_.isAggregateType()) {
        std::vector<Domain::Ptr> subOffsets(indices.begin() + 1, indices.end());
        for (auto&& it : locations_) {
            auto gepResult = it->gep(type, subOffsets);
            result = result ?
                     result->join(gepResult) :
                     gepResult;
        }
    } else if (elementType_.isPointerTy()) {
        if (indices.size() == 1) {
            auto&& intOffset = llvm::dyn_cast<IntegerInterval>(indices.begin()->get());
            ASSERT(intOffset->isConstant(0), "Big offset in integer");

            return factory_->getPointer(type, locations_);

        } else {
            std::vector<Domain::Ptr> subOffsets(indices.begin() + 1, indices.end());
            for (auto&& it : locations_) {
                result = result ?
                         result->join(it->gep(type, subOffsets)) :
                         it;
            }
        }

    } else {
        for (auto&& it : locations_) {
            auto gepResult = it->gep(type, indices);
            result = result ?
                     result->join(gepResult) :
                     gepResult;
        }
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

