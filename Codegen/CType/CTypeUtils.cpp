//
// Created by belyaev on 10/20/15.
//


#include "Codegen/CType/CTypeUtils.h"
#include "Codegen/CType/CType.def"
#include "Util/cast.hpp"
#include "Util/functional.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

CType::Ptr CTypeUtils::stripAllAliasing(CType::Ptr type) {
    while(auto alias = llvm::dyn_cast<CAlias>(type)) {
        type = alias->getOriginal().get();
    }
    return type;
}

llvm::Signedness CTypeUtils::getSignedness(CType::Ptr type) {
    type = stripAllAliasing(type);

    // pointers are inherently unsigned
    if(llvm::isa<CPointer>(type)) return llvm::Signedness::Unsigned;

    auto integer = llvm::dyn_cast<CInteger>(type);
    if(!integer) return llvm::Signedness::Unknown;
    else return integer->getSignedness();
}

CStructMember::Ptr CTypeUtils::getField(CType::Ptr struct_, const std::string& fieldName) {
    struct_ = stripAllAliasing(struct_);
    if(auto csp = dyn_cast<CStruct>(struct_)) {
        auto&& members = csp->getElements();
        auto mem_ptr = util::viewContainer(members)
            .filter(LAM(mem, mem.getName() == fieldName))
            .map(ops::take_pointer)
            .first_or(nullptr);
        if(mem_ptr) return CStructMember::Ptr(struct_, mem_ptr);
    }
    return nullptr;
}

CType::Ptr CTypeUtils::indexType(CType::Ptr base) {
    base = stripAllAliasing(base);
    if(auto cptr = dyn_cast<CPointer>(base)) {
        return cptr->getElement().get();
    }

    if(auto carr = dyn_cast<CArray>(base)) {
        return carr->getElement().get();
    }

    return nullptr;
}

CType::Ptr CTypeUtils::loadType(CType::Ptr base) {
    base = stripAllAliasing(base);
    if(auto cptr = dyn_cast<CPointer>(base)) {
        return cptr->getElement().get();
    }
    if(auto cptr = dyn_cast<CArray>(base)) {
        return cptr->getElement().get();
    }
    return nullptr;
}

CType::Ptr CTypeUtils::commonType(CTypeFactory& ctx, CType::Ptr lhv, CType::Ptr rhv) {
    if(lhv == rhv) return lhv;
    // we do not really handle aliasing complexity here, but for our purposes it's ok
    lhv = stripAllAliasing(lhv);
    rhv = stripAllAliasing(rhv);
    if(lhv == rhv) return lhv;

    lhv = decayType(ctx, lhv);
    rhv = decayType(ctx, rhv);
    if(lhv == rhv) return lhv;

    if(isa<CPointer>(lhv) && isa<CPointer>(rhv)) {
        auto&& lhvp = dyn_cast<CPointer>(lhv);
        auto&& rhvp = dyn_cast<CPointer>(rhv);
        auto&& lhve = lhvp->getElement().get();
        auto&& rhve = rhvp->getElement().get();
        auto void_ = ctx.getVoid();
        if(lhve == void_ || rhve == void_) {
            return ctx.getPointer(void_);
        }
        if(stripAllAliasing(lhve) == stripAllAliasing(rhve)) {
            return ctx.getPointer(stripAllAliasing(lhve));
        }
        return ctx.getPointer(void_);
    }

    if(isa<CInteger>(lhv) && isa<CInteger>(rhv)) {
        auto&& lhvi = dyn_cast<CInteger>(lhv);
        auto&& rhvi = dyn_cast<CInteger>(rhv);
        return lhvi->getBitsize() > rhvi->getBitsize()? lhv
            : lhvi->getBitsize() < rhvi->getBitsize()? rhv
            : lhvi->getSignedness() == llvm::Signedness::Unknown? rhv
            : rhvi->getSignedness() == llvm::Signedness::Unknown? lhv
            : lhvi->getSignedness() == llvm::Signedness::Unsigned? lhv
            : rhv;
    }

    if(is_one_of<CPointer, CFloat>(lhv) && isa<CInteger>(rhv)) return lhv;
    if(isa<CInteger>(lhv) && is_one_of<CPointer, CFloat>(rhv)) return rhv;

    return nullptr;
}

CType::Ptr CTypeUtils::decayType(CTypeFactory& ctx, CType::Ptr tp) {
    tp = stripAllAliasing(tp);
    if(auto&& array = dyn_cast<CArray>(tp)) {
        return ctx.getPointer(array->getElement());
    }
    return tp;
}
} /* namespace borealis */

#include "Util/unmacros.h"
