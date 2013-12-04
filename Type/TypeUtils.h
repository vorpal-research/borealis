/*
 * TypeUtils.h
 *
 *  Created on: Nov 25, 2013
 *      Author: ice-phoenix
 */

#ifndef TYPEUTILS_H_
#define TYPEUTILS_H_

#include "Type/Type.def"
#include "Util/typeindex.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

struct TypeUtils {

    static bool isValid(Type::Ptr type) {
        return type->getClassTag() != class_tag<type::TypeError>();
    }

    static bool isInvalid(Type::Ptr type) {
        return ! isValid(type);
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
            for(const auto& fld : Rec->getBody()->get()) {
                ret += util::toString(fld.getIndex());
                ret += ": ";
                ret += util::toString(fld.getIds());
                ret += " ";
            }
            ret += "}";
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

};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* TYPEUTILS_H_ */
