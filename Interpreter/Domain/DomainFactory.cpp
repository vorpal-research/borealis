//
// Created by abdullin on 2/7/17.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "DomainFactory.h"
#include "Util.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {

DomainFactory::DomainFactory() : ObjectLevelLogging("domain") {}

DomainFactory::~DomainFactory() {
    auto&& info = infos();
    info << "DomainFactory statistics:" << endl;
    info << "Integers: " << ints_.size() << endl;
    info << "Floats: " << floats_.size() << endl;
    info << "Pointers: " << ptrs_.size() << endl;
    info << endl;
}


/*  General */
Domain::Ptr DomainFactory::getTop(const llvm::Type& type) {
    if (type.isVoidTy()) {
        return nullptr;
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(Domain::TOP, intType->getBitWidth());
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(Domain::TOP, semantics);
    } else if (type.isPointerTy()) {
        return getPointer(Domain::TOP);
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr DomainFactory::getBottom(const llvm::Type& type) {
    if (type.isVoidTy()) {
        return nullptr;
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(Domain::BOTTOM, intType->getBitWidth());
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(Domain::BOTTOM, semantics);
    } else if (type.isPointerTy()) {
        return getPointer(Domain::BOTTOM);
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr borealis::absint::DomainFactory::get(const llvm::Value* val) {
    auto&& type = *val->getType();
    if (type.isVoidTy()) {
        return nullptr;
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(intType->getBitWidth());
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(semantics);
    } else if (type.isPointerTy()) {
        return getPointer();
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr DomainFactory::get(const llvm::Constant* constant) {
    if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return getInteger(intConstant->getValue());
    } else if (auto&& floatConstant = llvm::dyn_cast<llvm::ConstantFP>(constant)) {
        return getFloat(llvm::APFloat(floatConstant->getValueAPF()));
    } else if (auto&& ptrConstant = llvm::dyn_cast<llvm::ConstantPointerNull>(constant)) {
        return getPointer(false);
    } else {
        return get(llvm::cast<llvm::Value>(constant));
    }
}

Domain::Ptr DomainFactory::cached(const IntegerInterval::ID& key) {
    if (ints_.find(key) == ints_.end()) {
        ints_[key] = Domain::Ptr{ new IntegerInterval(this, key) };
    }
    return ints_[key];
}

Domain::Ptr DomainFactory::cached(const FloatInterval::ID& key) {
    if (floats_.find(key) == floats_.end()) {
        floats_[key] = Domain::Ptr{ new FloatInterval(this, key) };
    }
    return floats_[key];
}

Domain::Ptr DomainFactory::cached(const Pointer::ID& key) {
    if (ptrs_.find(key) == ptrs_.end()) {
        ptrs_[key] = Domain::Ptr{ new Pointer(this, key) };
    }
    return ptrs_[key];
}


/* Integer */
Domain::Ptr DomainFactory::getIndex(uint64_t indx) {
    return getInteger(llvm::APInt(32, indx, false), false);
}

Domain::Ptr DomainFactory::getInteger(unsigned width, bool isSigned) {
    return getInteger(Domain::BOTTOM, width, isSigned);
}

Domain::Ptr DomainFactory::getInteger(const llvm::APInt& val, bool isSigned) {
    return getInteger(val, val, isSigned);
}

Domain::Ptr DomainFactory::getInteger(Domain::Value value, unsigned width, bool isSigned) {
    return cached(std::make_tuple(value,
                                  isSigned,
                                  llvm::APInt(width, 0, isSigned),
                                  llvm::APInt(width, 0, isSigned)));
}

Domain::Ptr DomainFactory::getInteger(const llvm::APInt& from, const llvm::APInt& to, bool isSigned) {
    return cached(std::make_tuple(Domain::VALUE,
                                  isSigned,
                                  from,
                                  to));
}


/* Float */
Domain::Ptr DomainFactory::getFloat(const llvm::fltSemantics& semantics) {
    return getFloat(Domain::BOTTOM, semantics);
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& val) {
    return getFloat(val, val);
}

Domain::Ptr DomainFactory::getFloat(Domain::Value value, const llvm::fltSemantics& semantics) {
    return cached(std::make_tuple(value,
                                  llvm::APFloat(semantics),
                                  llvm::APFloat(semantics)));
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& from, const llvm::APFloat& to) {
    return cached(std::make_tuple(Domain::VALUE,
                                  llvm::APFloat(from),
                                  llvm::APFloat(to)));
}


/* Pointer */
Domain::Ptr DomainFactory::getPointer() {
    return getPointer(Domain::BOTTOM);
}

Domain::Ptr DomainFactory::getPointer(bool isValid) {
    return getPointer(isValid ? Pointer::VALID : Pointer::NON_VALID);
}

Domain::Ptr DomainFactory::getPointer(Domain::Value value) {
    return cached(std::make_tuple(value, Pointer::NON_VALID));
}

Domain::Ptr DomainFactory::getPointer(Pointer::Status status) {
    return cached(std::make_tuple(Domain::VALUE, status));
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"