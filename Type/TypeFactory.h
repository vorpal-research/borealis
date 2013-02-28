/*
 * TypeFactory.h
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include "Type/Type.h"
#include "Type/Bool.h"
#include "Type/Float.h"
#include "Type/Integer.h"
#include "Type/Pointer.h"
#include "Type/TypeError.h"
#include "Type/UnknownType.h"

namespace borealis {

class TypeFactory {
    TypeFactory();
    ~TypeFactory();

    Type::Ptr theBool;
    Type::Ptr theInteger;
    Type::Ptr theFloat;
    Type::Ptr theUnknown;
    std::map<Type::Ptr, Type::Ptr> pointers;
    std::map<std::string, Type::Ptr> errors;

public:
    static TypeFactory& getInstance() {
        static TypeFactory instance;
        return instance;
    }

    Type::Ptr getBool() {
        if(!theBool) theBool = Type::Ptr(new Bool());
        return theBool;
    }

    Type::Ptr getInteger() {
        if(!theInteger) theInteger = Type::Ptr(new Integer());
        return theInteger;
    }

    Type::Ptr getFloat() {
        if(!theFloat) theFloat = Type::Ptr(new Float());
        return theFloat;
    }

    Type::Ptr getUnknown() {
        if(!theUnknown) theUnknown = Type::Ptr(new UnknownType());
        return theUnknown;
    }

    Type::Ptr getPointer(Type::Ptr to) {
        if(!pointers.count(to)) pointers[to] = Type::Ptr(new Pointer(to));
        return pointers[to];
    }

    Type::Ptr getTypeError(const std::string& message) {
        if(!errors.count(message)) errors[message] = Type::Ptr(new TypeError(message));
        return errors[message];
    }

    bool isValid(Type::Ptr type) {
        return type->getId() != type_id<TypeError>();
    }

    Type::Ptr cast(llvm::Type* type) {
        using borealis::util::toString;

        if(type->isIntegerTy())
            return (type->getIntegerBitWidth() == 1) ? getBool() : getInteger();
        else if(type->isFloatTy())
            return getFloat();
        else if(type->isPointerTy())
            return getPointer(cast(type->getPointerElementType()));
        else
            return getTypeError("Unsupported llvm type: " + toString(*type));
    }

#include "Util/macros.h"
    std::string toString(const Type& type) {
        using llvm::isa;
        using llvm::dyn_cast;

        if(isa<Integer>(type)) return "Integer";
        if(isa<Float>(type)) return "Float";
        if(isa<Bool>(type)) return "Bool";
        if(isa<UnknownType>(type)) return "Unknown";
        if(auto* Ptr = dyn_cast<Pointer>(&type)) return toString(*Ptr->getPointed()) + "*";
        if(auto* Err = dyn_cast<TypeError>(&type)) return "<Type Error>: " + Err->getMessage();

        BYE_BYE(std::string, "Unknown type")
    }
#include "Util/unmacros.h"

};

std::ostream& operator<<(std::ostream& ost, const Type& tp);

} /* namespace borealis */

#endif /* TYPEFACTORY_H_ */
