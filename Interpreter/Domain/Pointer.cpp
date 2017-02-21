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

Pointer::Pointer(const borealis::absint::DomainFactory* factory) :
        Domain(BOTTOM, Type::Pointer, factory),
        status_(NON_VALID) {}

Pointer::Pointer(Domain::Value value, const DomainFactory* factory) :
        Domain(value, Type::Pointer, factory) {
    if (value == BOTTOM) status_ = NON_VALID;
}

Pointer::Pointer(const DomainFactory* factory, Pointer::Status status) :
        Domain(VALUE, Type::Pointer, factory),
        status_(status) {}

Pointer::Pointer(const Pointer& other) :
        Domain(other.value_, Type::Pointer, other.factory_),
        status_(other.status_) {}

bool Pointer::equals(const Domain* other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other);
    if (not value) return false;

    if (this->isBottom() && other->isBottom()) return true;
    if (this->isTop() && other->isTop()) return true;

    return value && this->getStatus() == value->getStatus();
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
        return Domain::Ptr{ clone() };
    } else if (this->isBottom()) {
        return Domain::Ptr{ value->clone() };
    } else if (this->isTop() || value->isTop()) {
        return factory_->getPointer(TOP);
    } else {
        return this->getStatus() == value->getStatus() ?
               factory_->getPointer(this->getStatus()) :
               factory_->getPointer(TOP);
    }
}
// TODO: check this
Domain::Ptr Pointer::meet(Domain::Ptr other) const {
    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer meet");

    if (this->isBottom() || value->isBottom()) {
        return Domain::Ptr{ clone() };
    } else if (this->isTop() || value->isTop()) {
        return factory_->getPointer(TOP);
    } else {
        return this->getStatus() == value->getStatus() ?
               factory_->getPointer(this->getStatus()) :
               factory_->getPointer(TOP);
    }
}

Domain::Ptr Pointer::widen(Domain::Ptr) const {
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
    if (not isValid()) {
        infos() << "Warning: trying to load from non-valid ptr" << endl;
    }
    return factory_->get(type, (isValid()) ? TOP : BOTTOM);
}

Domain::Ptr Pointer::store(Domain::Ptr, const std::vector<Domain::Ptr>&) const {
    if (not isValid()) {
        infos() << "Warning: trying to write to non-valid ptr" << endl;
    }
    return shared_from_this();
}

Domain::Ptr Pointer::gep(const llvm::Type& type, const std::vector<Domain::Ptr>&) const {
    if (not isValid()) {
        infos() << "Warning: trying to load from non-valid ptr" << endl;
    }
    return factory_->get(type, (isValid()) ? TOP : BOTTOM);
}

Domain::Ptr Pointer::ptrtoint(const llvm::Type& type) const {
    return factory_->get(type, TOP);
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
        return factory_->getInteger(1, false);
    } else if (this->isTop() || other->isTop()) {
        return factory_->getInteger(TOP, 1, false);
    }

    auto&& value = llvm::dyn_cast<Pointer>(other.get());
    ASSERT(value, "Nullptr in pointer");

    switch (operation) {
        case llvm::CmpInst::ICMP_EQ:
            if (this->isValid() ^ value->isValid())
                return getBool(false);
            else
                return factory_->getInteger(TOP, 1, false);

        case llvm::CmpInst::ICMP_NE:
            if (this->isValid() ^ value->isValid())
                return getBool(true);
            else
                return factory_->getInteger(TOP, 1, false);

        default:
            UNREACHABLE("Unknown operation in icmp");
    }
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"

