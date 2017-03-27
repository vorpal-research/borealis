//
// Created by abdullin on 2/17/17.
//

#include "DomainFactory.h"
#include "Pointer.h"
#include "Util/hash.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

Pointer::Pointer(DomainFactory* factory) :
        Pointer(BOTTOM, factory) {}

Pointer::Pointer(Domain::Value value, DomainFactory* factory) :
        Pointer(factory, std::make_tuple(value, NON_VALID)) {}

Pointer::Pointer(DomainFactory* factory, Pointer::Status status) :
        Pointer(factory, std::make_tuple(VALUE, status)) {}

Pointer::Pointer(DomainFactory* factory, Pointer::ID id) :
        Domain(std::get<0>(id), Type::Pointer, factory),
        status_(std::get<1>(id)) {
    if (value_ == BOTTOM) status_ = NON_VALID;
}

Pointer::Pointer(const Pointer& other) :
        Domain(other.value_, Type::Pointer, other.factory_),
        status_(other.status_) {}

bool Pointer::equals(const Domain* other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other);
    if (not value) return false;
    if (this == other) return true;

    if (this->isBottom() && other->isBottom()) return true;
    if (this->isTop() && other->isTop()) return true;

    return this->getStatus() == value->getStatus();
}

bool Pointer::operator<(const Domain& other) const {
    auto&& value = llvm::dyn_cast<Pointer>(&other);
    ASSERT(value, "Comparing domains of different type");

    if (this->isBottom() && not value->isBottom()) return true;
    return this->isValue() && value->isTop();
}

Domain::Ptr Pointer::join(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer join");

    if (value->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return value->shared_from_this();
    } else if (this->isTop() || value->isTop()) {
        return factory_->getPointer(TOP);
    } else {
        return this->getStatus() == value->getStatus() ?
               factory_->getPointer(this->getStatus()) :
               factory_->getPointer(TOP);
    }
}

Domain::Ptr Pointer::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer meet");

    if (this->isBottom() || value->isBottom()) {
        return shared_from_this();
    } else if (this->isTop() || value->isTop()) {
        return factory_->getPointer(TOP);
    } else {
        return this->getStatus() == value->getStatus() ?
               factory_->getPointer(this->getStatus()) :
               factory_->getPointer(TOP);
    }
}

Domain::Ptr Pointer::widen(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer domain");

    if (value->isBottom()) {
        return shared_from_this();
    } else if (this->isBottom()) {
        return value->shared_from_this();
    }

    return factory_->getPointer(TOP);
}

Domain::Ptr Pointer::narrow(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer domain");

    if (this->isBottom()) {
        return shared_from_this();
    } else if (value->isBottom()) {
        return value->shared_from_this();
    }

    return factory_->getPointer(TOP);
}

Pointer::Status Pointer::getStatus() const {
    return status_;
}

size_t Pointer::hashCode() const {
    return util::hash::simple_hash_value(value_, getType(), getStatus());
}

std::string Pointer::toString() const {
    if (isBottom()) return "**Bottom**";
    if (isTop()) return "**Top**";
    else {
        if (getStatus() == VALID) return "**Valid**";
        else return "**Non-valid**";
    }
}

Domain* Pointer::clone() const {
    return new Pointer(*this);
}

Domain::Ptr Pointer::load(const llvm::Type& type, const std::vector<Domain::Ptr>&) const {
    return factory_->get(type, TOP);
}

Domain::Ptr Pointer::store(Domain::Ptr, const std::vector<Domain::Ptr>&) const {
    return shared_from_this();
}

Domain::Ptr Pointer::gep(const llvm::Type& type, const std::vector<Domain::Ptr>&) const {
    return factory_->get(type, TOP);
}

Domain::Ptr Pointer::ptrtoint(const llvm::Type& type) const {
    return factory_->get(type, TOP);
}

Domain::Ptr Pointer::bitcast(const llvm::Type& type) const {
    if (type.isPointerTy()) {
        return shared_from_this();
    } else {
        UNREACHABLE("Bitcast to unknown type");
    }
}

bool Pointer::isValid() const {
    return isValue() && status_ == VALID;
}

Domain::Ptr Pointer::icmp(Domain::Ptr other, llvm::CmpInst::Predicate operation) const {
    auto&& getBool = [&] (bool val) -> Domain::Ptr {
        llvm::APSInt retval(1, true);
        if (val) retval = 1;
        else retval = 0;
        return factory_->getInteger(retval);
    };

    if (this->isBottom() || other->isBottom()) {
        return factory_->getInteger(1);
    } else if (this->isTop() || other->isTop()) {
        return factory_->getInteger(TOP, 1);
    }

    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isValid() ^ value->isValid())
                return getBool(false);
            else
                return factory_->getInteger(TOP, 1);

        case llvm::CmpInst::ICMP_NE:
            if (this->isValid() ^ value->isValid())
                return getBool(true);
            else
                return factory_->getInteger(TOP, 1);

        default:
            return factory_->getInteger(TOP, 1);
    }
}

bool Pointer::classof(const Domain* other) {
    return other->getType() == Domain::Pointer;
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

