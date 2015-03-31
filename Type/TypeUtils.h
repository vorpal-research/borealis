/*
 * TypeUtils.h
 *
 *  Created on: Nov 25, 2013
 *      Author: ice-phoenix
 */

#ifndef TYPEUTILS_H_
#define TYPEUTILS_H_

#include <Util/functional.hpp>
#include "Type/Type.def"
#include "Util/typeindex.hpp"
#include "Util/util.h"
#include "Util/functional.hpp"

#include "Util/macros.h"

namespace borealis {

struct TypeUtils {

    static bool isValid(Type::Ptr type) {
        return type->getClassTag() != class_tag<type::TypeError>();
    }

    static bool isInvalid(Type::Ptr type) {
        return not isValid(type);
    }

    static bool isUnknown(Type::Ptr type) {
        return type->getClassTag() == class_tag<type::UnknownType>();
    }

    static std::string toString(const Type& type) {
        using llvm::isa;
        using llvm::dyn_cast;

        if(isa<type::Integer>(type)) return "Integer";
        if(isa<type::Float>(type)) return "Float";
        if(isa<type::Bool>(type)) return "Bool";
        if(isa<type::UnknownType>(type)) return "Unknown";
        if(auto* Ptr = dyn_cast<type::Pointer>(&type)) return TypeUtils::toString(*Ptr->getPointed()) + "*";
        if(auto* Err = dyn_cast<type::TypeError>(&type)) return "<Type Error>: " + Err->getMessage();
        if(auto* Arr = dyn_cast<type::Array>(&type)) {
            std::string ret = TypeUtils::toString(*Arr->getElement()) + "[";
            for(const auto& size : Arr->getSize()) {
                ret += util::toString(size);
            }
            ret += "]";
            return std::move(ret);
        }
        if(auto* Rec = dyn_cast<type::Record>(&type)) {
            std::string ret = Rec->getName() + "{ ";

            ret += util::viewContainer(Rec->getBody()->get())
                   .map([](auto&& e) {
                        return util::toString(e.getIndex()) +
                               ": " +
                               util::toString(e.getIds());
                    })
            	   .reduce("", [](auto&& e1, auto&& e2) {
                        return e1 + ", " + e2;
                    });

            ret += " }";
            return std::move(ret);
        }
        if(auto* Fun = dyn_cast<type::Function>(&type)) {
            std::string ret = TypeUtils::toString(*Fun->getRetty()) + "( ";

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

    static unsigned long long getTypeSizeInElems(Type::Ptr type) {
        return TypeSizer().visit(type);
    }

    static unsigned long long getStructOffsetInElems(Type::Ptr type, unsigned idx) {
        if (auto* structType = llvm::dyn_cast<type::Record>(type)) {
            const auto& recordBody = structType->getBody()->get();
            auto res = 0U;
            for (auto i = 0U; i < idx; ++i) {
                res += getTypeSizeInElems(recordBody.at(i).getType());
            }
            return res;
        }

        BYE_BYE(unsigned long long, "Funk you!");
    }

    static llvm::Type* tryCastBack(llvm::LLVMContext& C, Type::Ptr type) {
        if(const type::Integer* intt = llvm::dyn_cast<type::Integer>(type)) {
            return llvm::Type::getIntNTy(C, intt->getBitsize());
        } else if(llvm::dyn_cast<type::Float>(type)) {
            return llvm::Type::getDoubleTy(C);
        } else if(llvm::dyn_cast<type::Bool>(type)) {
            return llvm::Type::getInt1Ty(C);
        } else if(const type::Array* tp = llvm::dyn_cast<type::Array>(type)) {
            auto sz = tp->getSize().getOrElse(0);
            return llvm::ArrayType::get(tryCastBack(C, tp->getElement()), sz);
        } else if(const type::Pointer* tp = llvm::dyn_cast<type::Pointer>(type)) {
            return llvm::PointerType::get(tryCastBack(C, tp->getPointed()), 0);
        } else if(const type::Function* tp = llvm::dyn_cast<type::Function>(type)) {
            return llvm::FunctionType::get(
                tryCastBack(C, tp->getRetty()),
                util::viewContainer(tp->getArgs()).map(LAM(arg, tryCastBack(C, arg))).toVector(),
                false
            );
        } else if(const type::Record* tp = llvm::dyn_cast<type::Record>(type)) {
            return llvm::StructType::get(
                C,
                util::viewContainer(tp->getBody()->get()).map(LAM(field, tryCastBack(C, field.getType()))).toVector()
            );
        }
        BYE_BYE(llvm::Type*, "Unsupported type");
    }

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TYPEUTILS_H_ */
