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

DomainFactory::DomainFactory(const Andersen* aa) : ObjectLevelLogging("domain"), aa_(aa) {}

DomainFactory::~DomainFactory() {
    auto&& info = infos();
    info << "DomainFactory statistics:" << endl;
    info << "Integers: " << ints_.size() << endl;
    info << "Floats: " << floats_.size() << endl;
    info << endl;
}

#define GENERATE_GET_VALUE(VALUE) \
    if (type.isVoidTy()) { \
        return nullptr; \
    } else if (type.isIntegerTy()) { \
        auto&& intType = llvm::cast<llvm::IntegerType>(&type); \
        return getInteger(VALUE, intType->getBitWidth()); \
    } else if (type.isFloatingPointTy()) { \
        auto& semantics = util::getSemantics(type); \
        return getFloat(VALUE, semantics); \
    } else if (type.isArrayTy()) { \
        auto&& arrayType = llvm::cast<llvm::ArrayType>(type); \
        return getArray(VALUE, arrayType); \
    } else if (type.isPointerTy()) { \
        return getPointer(VALUE, *type.getPointerElementType()); \
    } else { \
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl; \
        return nullptr; \
    }


/*  General */
Domain::Ptr DomainFactory::getTop(const llvm::Type& type) {
    GENERATE_GET_VALUE(Domain::TOP);
}

Domain::Ptr DomainFactory::getBottom(const llvm::Type& type) {
    GENERATE_GET_VALUE(Domain::BOTTOM);
}

Domain::Ptr DomainFactory::get(const llvm::Value* val) {
    auto&& type = *val->getType();
    if (type.isVoidTy()) {
        return nullptr;
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(intType->getBitWidth());
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(semantics);
    } else if (type.isArrayTy()) {
        auto&& arrayType = llvm::cast<llvm::ArrayType>(type);
        return getArray(arrayType);
    } else if (type.isPointerTy()) {
        std::vector<const llvm::Value*> aliases;
        aa_->getPointsToSet(val, aliases);

        auto&& location = get(*type.getPointerElementType());
        return getPointer(*type.getPointerElementType(), {location});
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr DomainFactory::get(const llvm::Type& type) {
    if (type.isVoidTy()) {
        return nullptr;
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(intType->getBitWidth());
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(semantics);
    } else if (type.isArrayTy()) {
        auto&& arrayType = llvm::cast<llvm::ArrayType>(type);
        return getArray(arrayType);
    } else if (type.isPointerTy()) {
        auto&& location = get(*type.getPointerElementType());
        return getPointer(*type.getPointerElementType(), {location});
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
        return getPointer(*constant->getType());
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
    return Domain::Ptr{ new IntegerInterval(this, std::make_tuple(value,
                                                                  isSigned,
                                                                  llvm::APInt(width, 0, isSigned),
                                                                  llvm::APInt(width, 0, isSigned))) };
}

Domain::Ptr DomainFactory::getInteger(const llvm::APInt& from, const llvm::APInt& to, bool isSigned) {
    return Domain::Ptr{ new IntegerInterval(this, std::make_tuple(Domain::VALUE,
                                                                  isSigned,
                                                                  from,
                                                                  to)) };
}


/* Float */
Domain::Ptr DomainFactory::getFloat(const llvm::fltSemantics& semantics) {
    return getFloat(Domain::BOTTOM, semantics);
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& val) {
    return getFloat(val, val);
}

Domain::Ptr DomainFactory::getFloat(Domain::Value value, const llvm::fltSemantics& semantics) {
    return Domain::Ptr{ new FloatInterval(this, std::make_tuple(value,
                                                                llvm::APFloat(semantics),
                                                                llvm::APFloat(semantics))) };
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& from, const llvm::APFloat& to) {
    return Domain::Ptr{ new FloatInterval(this, std::make_tuple(Domain::VALUE,
                                                                llvm::APFloat(from),
                                                                llvm::APFloat(to))) };
}


/* Pointer */
Domain::Ptr DomainFactory::getPointer(Domain::Value value, const llvm::Type& elementType) {
    return Domain::Ptr{ new Pointer(value, this, elementType) };
}

Domain::Ptr DomainFactory::getPointer(const llvm::Type& elementType) {
    return Domain::Ptr{ new Pointer(Domain::BOTTOM, this, elementType) };
}

Domain::Ptr DomainFactory::getPointer(const llvm::Type& elementType, const Pointer::Locations& locations) {
    return Domain::Ptr{ new Pointer(this, elementType, locations) };
}

/* Array */
Domain::Ptr DomainFactory::getArray(Domain::Value value, const llvm::ArrayType& type) {
    return Domain::Ptr{ new Array(value, this, *type.getElementType()) };
}

Domain::Ptr DomainFactory::getArray(const llvm::ArrayType& type) {
    return Domain::Ptr{ new Array(this, *type.getElementType(), getIndex(type.getArrayNumElements())) };
}

Domain::Ptr DomainFactory::getArray(const llvm::ArrayType& type, const Array::Elements& elements) {
    return Domain::Ptr{ new Array(this, *type.getElementType(), elements) };
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"