//
// Created by abdullin on 2/7/17.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "DomainFactory.h"
#include "Interpreter/Util.h"
#include "Util/cast.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

DomainFactory::DomainFactory() : ObjectLevelLogging("domain") {}

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
    } else if (type.isAggregateType()) { \
        return getAggregateObject(type); \
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
    /// Void type - do nothing
    if (type.isVoidTy()) {
        return nullptr;
    /// Integer
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(intType->getBitWidth());
    /// Float
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(semantics);
    /// Aggregate type (Array or Struct)
    } else if (type.isAggregateType()) {
        return getAggregateObject(type);
    /// Pointer
    } else if (type.isPointerTy()) {
        return getInMemory(type);
    /// Otherwise
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr DomainFactory::get(const llvm::Constant* constant) {
    /// Integer
    if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return getInteger(intConstant->getValue());
    /// Float
    } else if (auto&& floatConstant = llvm::dyn_cast<llvm::ConstantFP>(constant)) {
        return getFloat(llvm::APFloat(floatConstant->getValueAPF()));
    /// Nullpointer
    } else if (auto&& ptrConstant = llvm::dyn_cast<llvm::ConstantPointerNull>(constant)) {
        return getPointer(*constant->getType());
    /// Constant Array
    } else if (auto&& sequential = llvm::dyn_cast<llvm::ConstantDataSequential>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < sequential->getNumElements(); ++i) {
            auto element = get(sequential->getElementAsConstant(i));
            if (not element) {
                errs() << "Cannot create constant: " << util::toString(*sequential->getElementAsConstant(i)) << endl;
                return nullptr;
            }
            elements.push_back(element);
        }
        return getAggregateObject(*constant->getType(), elements);
    /// Constant Struct
    } else if (auto&& structType = llvm::dyn_cast<llvm::ConstantStruct>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < structType->getNumOperands(); ++i) {
            auto element = get(structType->getAggregateElement(i));
            if (not element) {
                errs() << "Cannot create constant: " << util::toString(*structType->getAggregateElement(i)) << endl;
                return nullptr;
            }
            elements.push_back(element);
        }
        return getAggregateObject(*constant->getType(), elements);
    /// otherwise
    } else {
        auto value = llvm::cast<llvm::Value>(constant);
        errs() << "Unknown constant: " << util::toString(*value) << endl;
        return getTop(*value->getType());
    }
}

Domain::Ptr DomainFactory::getInMemory(const llvm::Type& type) {
    /// Void type - do nothing
    if (type.isVoidTy()) {
        return nullptr;
    /// Simple type - allocating like array
    } else if (type.isIntegerTy() || type.isFloatingPointTy()) {
        auto&& arrayType = llvm::ArrayType::get(const_cast<llvm::Type*>(&type), 1);
        return getInMemory(*arrayType);
    /// Struct or Array type
    } else if (type.isAggregateType()) {
        return getAggregateObject(type);
    /// Pointer
    } else if (type.isPointerTy()) {
        auto&& location = getInMemory(*type.getPointerElementType());
        return getPointer(*type.getPointerElementType(), { {getIndex(0), location} });
    /// Otherwise
    } else {
        errs() << "Creating domain of unknown type <" << util::toString(type) << ">" << endl;
        return nullptr;
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
    return getInteger(llvm::APInt(64, indx, false), false);
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
Domain::Ptr DomainFactory::getPointer(Domain::Value value, const llvm::Type& elementType) {
    return Domain::Ptr{ new Pointer(value, this, elementType) };
}

Domain::Ptr DomainFactory::getPointer(const llvm::Type& elementType) {
    return Domain::Ptr{ new Pointer(Domain::BOTTOM, this, elementType) };
}

Domain::Ptr DomainFactory::getPointer(const llvm::Type& elementType, const Pointer::Locations& locations) {
    return Domain::Ptr{ new Pointer(this, elementType, locations) };
}

/* AggregateObject */
Domain::Ptr DomainFactory::getAggregateObject(const llvm::Type& type) {
    return getAggregateObject(Domain::VALUE, type);
}

Domain::Ptr DomainFactory::getAggregateObject(Domain::Value value, const llvm::Type& type) {
    if (type.isArrayTy()) {
        return Domain::Ptr{new AggregateObject(value, this,
                                               *type.getArrayElementType(),
                                               getIndex(type.getArrayNumElements()))};
    } else if (type.isStructTy()) {
        AggregateObject::Types types;
        for (auto i = 0U; i < type.getStructNumElements(); ++i)
            types[i] = type.getStructElementType(i);

        return Domain::Ptr{new AggregateObject(value, this,
                                               types,
                                               getIndex(type.getStructNumElements()))};
    }
    UNREACHABLE("Unknown aggregate type: " + util::toString(type));
}

Domain::Ptr DomainFactory::getAggregateObject(const llvm::Type& type, std::vector<Domain::Ptr> elements) {
    if (type.isArrayTy()) {
        AggregateObject::Elements elementMap;
        for (auto i = 0U; i < elements.size(); ++i) {
            elementMap[i] = getMemoryObject(elements[i]);
        }
        return Domain::Ptr { new AggregateObject(this, *type.getArrayElementType(), elementMap) };

    } else if (type.isStructTy()) {
        AggregateObject::Types types;
        AggregateObject::Elements elementMap;
        for (auto i = 0U; i < type.getStructNumElements(); ++i) {
            types[i] = type.getStructElementType(i);
            elementMap[i] = getMemoryObject(elements[i]);
        }
        return Domain::Ptr { new AggregateObject(this, types, elementMap) };
    }
    UNREACHABLE("Unknown aggregate type: " + util::toString(type));
}

/* heap */
MemoryObject::Ptr DomainFactory::getMemoryObject(const llvm::Type& type) {
    return MemoryObject::Ptr{ new MemoryObject(getBottom(type)) };
}

MemoryObject::Ptr DomainFactory::getMemoryObject(Domain::Ptr value) {
    return MemoryObject::Ptr{ new MemoryObject(value) };
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"