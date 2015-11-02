//
// Created by ice-phoenix on 9/14/15.
//

#include "Type/TypeUtils.h"
#include "Type/TypeVisitor.hpp"
#include "Util/functional.hpp"
#include "Util/typeindex.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

bool TypeUtils::isConcrete(Type::Ptr type) {
    return type->getClassTag() != class_tag<type::TypeError>()
           and type->getClassTag() != class_tag<type::UnknownType>();
}

bool TypeUtils::isValid(Type::Ptr type) {
    return type->getClassTag() != class_tag<type::TypeError>();
}

bool TypeUtils::isInvalid(Type::Ptr type) {
    return not isValid(type);
}

bool TypeUtils::isUnknown(Type::Ptr type) {
    return type->getClassTag() == class_tag<type::UnknownType>();
}

std::string TypeUtils::toString(const Type& type) {
    using llvm::isa;
    using llvm::dyn_cast;

    if (auto Int = dyn_cast<type::Integer>(&type)) return "Integer(" + util::toString(Int->getBitsize()) + ")";
    if (isa<type::Float>(type)) return "Float";
    if (isa<type::Bool>(type)) return "Bool";
    if (isa<type::UnknownType>(type)) return "Unknown";
    if (auto* Ptr = dyn_cast<type::Pointer>(&type)) return TypeUtils::toString(*Ptr->getPointed()) + "*";
    if (auto* Err = dyn_cast<type::TypeError>(&type)) return "<Type Error>: " + Err->getMessage();
    if (auto* Arr = dyn_cast<type::Array>(&type)) {
        auto&& ret = TypeUtils::toString(*Arr->getElement()) + "[";
        for (auto&& size : Arr->getSize()) {
            ret += util::toString(size);
        }
        ret += "]";
        return std::move(ret);
    }
    if (auto* Rec = dyn_cast<type::Record>(&type)) {
        auto&& ret = Rec->getName() + "{ ";

        ret += util::viewContainer(Rec->getBody()->get())
            .map([](auto&& e) {
                return util::toString(e.getOffset()) +
                       ": " +
                       toString(*e.getType());
            })
            .reduce("", [](auto&& e1, auto&& e2) {
                return e1 + ", " + e2;
            });

        ret += " }";
        return std::move(ret);
    }
    if (auto* Fun = dyn_cast<type::Function>(&type)) {
        auto&& ret = TypeUtils::toString(*Fun->getRetty()) + "( ";

        ret += util::viewContainer(Fun->getArgs())
            .map(ops::dereference)
            .map(TypeUtils::toString)
            .reduce("", [](auto&& a, auto&& b) {
                return a + ", " + b;
            });

        ret += " )";
        return std::move(ret);
    }

    BYE_BYE(std::string, "Unknown type");
}

unsigned long long TypeUtils::getTypeSizeInElems(Type::Ptr type) {
    return TypeSizer().visit(type);
}

Type::Ptr TypeUtils::getPointerElementType(Type::Ptr type) {
    if (auto ptr = llvm::dyn_cast<type::Pointer>(type)) {
        return ptr->getPointed();
    } else {
        return nullptr;
    }
}

unsigned long long TypeUtils::getStructOffsetInElems(Type::Ptr type, unsigned idx) {
    if (auto* structType = llvm::dyn_cast<type::Record>(type)) {
        auto&& recordBody = structType->getBody()->get();
        auto&& res = 0U;
        for (auto&& i = 0U; i < idx; ++i) {
            res += getTypeSizeInElems(recordBody.at(i).getType());
        }
        return res;
    }

    BYE_BYE(unsigned long long, "Trying to get a field offset for non-struct type");
}

llvm::Type* TypeUtils::tryCastBack(llvm::LLVMContext& C, Type::Ptr type) {
    if (auto ti = llvm::dyn_cast<type::Integer>(type)) {
        return llvm::Type::getIntNTy(C, ti->getBitsize());
    } else if (llvm::dyn_cast<type::Float>(type)) {
        return llvm::Type::getDoubleTy(C);
    } else if (llvm::dyn_cast<type::Bool>(type)) {
        return llvm::Type::getInt1Ty(C);
    } else if (auto ta = llvm::dyn_cast<type::Array>(type)) {
        auto&& sz = ta->getSize().getOrElse(0);
        return llvm::ArrayType::get(tryCastBack(C, ta->getElement()), sz);
    } else if (auto tp = llvm::dyn_cast<type::Pointer>(type)) {
        return llvm::PointerType::get(tryCastBack(C, tp->getPointed()), 0);
    } else if (auto tf = llvm::dyn_cast<type::Function>(type)) {
        return llvm::FunctionType::get(
            tryCastBack(C, tf->getRetty()),
            util::viewContainer(tf->getArgs()).map(LAM(arg, tryCastBack(C, arg))).toVector(),
            false
        );
    } else if (auto tr = llvm::dyn_cast<type::Record>(type)) {
        return llvm::StructType::get(
            C,
            util::viewContainer(tr->getBody()->get()).map(LAM(field, tryCastBack(C, field.getType()))).toVector()
        );
    }
    BYE_BYE(llvm::Type*, "Unsupported type");
}

} // namespace borealis

#include "Util/unmacros.h"
