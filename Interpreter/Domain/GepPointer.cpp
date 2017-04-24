//
// Created by abdullin on 4/14/17.
//

#include "DomainFactory.h"
#include "GepPointer.h"
#include "IntegerInterval.h"
#include "Util.h"
#include "Util/collections.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

GepPointer::GepPointer(DomainFactory* factory, const llvm::Type& elementType, const GepPointer::Objects& objects)
        : Domain{VALUE, GEP, factory},
          elementType_(elementType),
          objects_(objects) {}

GepPointer::GepPointer(const GepPointer& other)
        : Domain{VALUE, GEP, other.factory_},
          elementType_(other.elementType_),
          objects_(other.objects_) {}

const llvm::Type& GepPointer::getElementType() const {
    return elementType_;
}

const GepPointer::Objects& GepPointer::getObjects() const {
    return objects_;
}

size_t GepPointer::hashCode() const {
    return 0;
}

std::string GepPointer::toString() const {
    std::stringstream ss;
    ss << "GEP " << util::toString(elementType_) << " [";
    for (auto&& it : objects_) {
        if (it) ss << std::endl << it->toString();
    }
    ss << std::endl << "]";
    return ss.str();
}

Domain* GepPointer::clone() const {
    return new GepPointer(*this);
}

bool GepPointer::classof(const Domain* other) {
    return other->getType() == Domain::GEP;
}

bool GepPointer::equals(const Domain* other) const {
    if (auto gep = llvm::dyn_cast<GepPointer>(other)) {
        if (this == gep) return true;

        return elementType_.getTypeID() == gep->elementType_.getTypeID() &&
               objects_ == gep->objects_;
    } else if (llvm::isa<Pointer>(other)) {
        return false;
    }
    return false;
}

bool borealis::absint::GepPointer::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr GepPointer::join(Domain::Ptr other) const {
    if (auto gep = llvm::dyn_cast<GepPointer>(other.get())) {
        ASSERT(elementType_.getTypeID() == gep->elementType_.getTypeID(), "Different types in gep join");

        Objects newObjects;
        auto insertToNew = [&newObjects](MemoryObject::Ptr obj) {
            newObjects.insert(obj);
        };
        util::viewContainer(gep->objects_).foreach(insertToNew);
        util::viewContainer(objects_).foreach(insertToNew);

        return factory_->getGepPointer(elementType_, newObjects);
    } else if (auto ptr = llvm::dyn_cast<Pointer>(other.get())) {
        ASSERT(elementType_.getTypeID() == ptr->getElementType().getTypeID(), "Different types in gep join");

        auto newPtr = ptr->clone();
        return newPtr->join(shared_from_this());
    }
    UNREACHABLE("Unknown type in gep join");
}

Domain::Ptr GepPointer::widen(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr GepPointer::meet(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr GepPointer::narrow(Domain::Ptr) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr GepPointer::load(const llvm::Type& type, const std::vector<Domain::Ptr>& offsets) const {
    auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
    ASSERT(intOffset->isConstant(0), "Too big offset in gep");

    if (isBottom())
        return factory_->getBottom(type);
    else if (isTop())
        return factory_->getTop(type);

    auto result = factory_->getBottom(elementType_);
    if (offsets.size() == 1) {
        ASSERT(elementType_.getTypeID() == type.getTypeID(), "Different load types");
        for (auto&& it : objects_)
            result = result->join(it->load());

    } else {
        std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
        for (auto&& it : objects_)
            result = result->join(it->load()->load(type, subOffsets));
    }
    return result;
}

void GepPointer::store(Domain::Ptr value, const std::vector<Domain::Ptr>& offsets) const {
    auto&& intOffset = llvm::dyn_cast<IntegerInterval>(offsets.begin()->get());
    ASSERT(intOffset->isConstant(0), "Too big offset in gep");

    if (offsets.size() == 1) {
        for (auto&& it : objects_) {
            it->store(value);
        }

    } else {
        std::vector<Domain::Ptr> subOffsets(offsets.begin() + 1, offsets.end());
        for (auto&& it : objects_)
            it->load()->store(value, subOffsets);
    }
}

Domain::Ptr GepPointer::gep(const llvm::Type&, const std::vector<Domain::Ptr>&) const {
    UNREACHABLE("Unimplemented, sorry...");
}

Domain::Ptr GepPointer::ptrtoint(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr GepPointer::bitcast(const llvm::Type& type) const {
    return factory_->getTop(type);
}

Domain::Ptr GepPointer::icmp(Domain::Ptr, llvm::CmpInst::Predicate) const {
    return factory_->getInteger(TOP, 1, false);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

