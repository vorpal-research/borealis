/*
 * TypeFactory.h
 *
 *  Created on: Feb 21, 2013
 *      Author: belyaev
 */

#ifndef TYPEFACTORY_H_
#define TYPEFACTORY_H_

#include "Type/Type.def"
#include "Util/util.h"

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
        if(!theBool) theBool = Type::Ptr(new type::Bool());
        return theBool;
    }

    Type::Ptr getInteger() {
        if(!theInteger) theInteger = Type::Ptr(new type::Integer());
        return theInteger;
    }

    Type::Ptr getFloat() {
        if(!theFloat) theFloat = Type::Ptr(new type::Float());
        return theFloat;
    }

    Type::Ptr getUnknown() {
        if(!theUnknown) theUnknown = Type::Ptr(new type::UnknownType());
        return theUnknown;
    }

    Type::Ptr getPointer(Type::Ptr to) {
        if(!pointers.count(to)) pointers[to] = Type::Ptr(new type::Pointer(to));
        return pointers[to];
    }

    Type::Ptr getTypeError(const std::string& message) {
        if(!errors.count(message)) errors[message] = Type::Ptr(new type::TypeError(message));
        return errors[message];
    }

    bool isValid(Type::Ptr type) {
        return type->getClassTag() != class_tag<type::TypeError>();
    }

    bool isUnknown(Type::Ptr type) {
        return type->getClassTag() == class_tag<type::UnknownType>();
    }

    Type::Ptr cast(const llvm::Type* type) {
        using borealis::util::toString;

        if(type->isIntegerTy())
            return (type->getIntegerBitWidth() == 1) ? getBool() : getInteger();
        else if(type->isFloatingPointTy())
            return getFloat();
        else if(type->isPointerTy())
            return getPointer(cast(type->getPointerElementType()));
        else if (type->isArrayTy())
            return getPointer(cast(type->getArrayElementType()));
        else
            return getTypeError("Unsupported llvm type: " + toString(*type));
    }

#include "Util/macros.h"
    std::string toString(const Type& type) {
        using llvm::isa;
        using llvm::dyn_cast;

        if(isa<type::Integer>(type)) return "Integer";
        if(isa<type::Float>(type)) return "Float";
        if(isa<type::Bool>(type)) return "Bool";
        if(isa<type::UnknownType>(type)) return "Unknown";
        if(auto* Ptr = dyn_cast<type::Pointer>(&type)) return toString(*Ptr->getPointed()) + "*";
        if(auto* Err = dyn_cast<type::TypeError>(&type)) return "<Type Error>: " + Err->getMessage();

        BYE_BYE(std::string, "Unknown type");
    }

    Type::Ptr merge(Type::Ptr one, Type::Ptr two) {
        if(!isValid(one)) return one;
        if(!isValid(two)) return two;

        if(isUnknown(one)) return two;
        if(isUnknown(two)) return one;
        if(one == two) return one;

        BYE_BYE(Type::Ptr, "Unmergeable types");
    }
#include "Util/unmacros.h"

};

std::ostream& operator<<(std::ostream& ost, Type::Ptr tp);

} /* namespace borealis */

#endif /* TYPEFACTORY_H_ */
