//
// Created by belyaev on 5/13/15.
//

#ifndef C_TYPE_FACTORY_H
#define C_TYPE_FACTORY_H

#include <queue>

#include "Codegen/llvm.h"
#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeContext.h"

#include "Codegen/CType/CType.def"

#include "Util/cast.hpp"
#include "Util/functional.hpp"
#include "Util/hash.hpp"

#include "Util/macros.h"

namespace borealis {

class CTypeFactory {

    CTypeContext::Ptr ctx;

    CType::Ptr record(CType::Ptr tp) {
        ctx->put(tp);
        return tp;
    }

    template<class Type, class ...Params>
    CTypeRef make(const std::string& name, Params&&... params) {
        if(ctx->has(name)) return getRef(name);
        else return getRef(record(borealis::util::make_shared_restricted<Type>(*this, name, std::forward<Params>(params)...)));
    }

    CTypeRef getRef(CTypeRef ptr) {
        return ptr;
    }

public:
    CTypeFactory() : ctx(std::make_shared<CTypeContext>()) {}
    CTypeFactory(CTypeFactory&&) = default;
    CTypeFactory(const CTypeFactory&) = default;
    CTypeFactory(CTypeContext::Ptr ctx) : ctx(ctx) {}

    CTypeRef getRef(const std::string& name) {
        return CTypeRef(name, ctx);
    }

    CTypeRef getRef(CType::Ptr tp) {
        ctx->put(tp);
        return CTypeRef(tp->getName(), ctx);
    }

    CType::Ptr getAlias(const std::string& name, CQualifier qual, CTypeRef tp) {
        return make<CAlias>(name, getRef(tp), qual);
    }

    CType::Ptr getTypedef(const std::string& name, CTypeRef tp) {
        return make<CAlias>(name, getRef(tp), CQualifier::TYPEDEF);
    }

    CType::Ptr getConst(CTypeRef tp) {
        auto&& inner = tp.get();
        auto&& isArrayLike = inner && is_one_of<CPointer, CArray>(inner);
        auto&& name = isArrayLike? tp.getName() + " const" : "const " + tp.getName();

        return make<CAlias>(name, getRef(tp), CQualifier::CONST);
    }

    CType::Ptr getVolatile(CTypeRef tp) {
        auto&& inner = tp.get();
        auto&& isArrayLike = inner && is_one_of<CPointer, CArray>(inner);
        auto&& name = isArrayLike? tp.getName() + " volatile" : "volatile " + tp.getName();

        return make<CAlias>(name, getRef(tp), CQualifier::VOLATILE);
    }

    CType::Ptr getArray(CTypeRef tp) {
        return make<CArray>(tp.getName() + "[]", getRef(tp), util::nothing());
    }

    CType::Ptr getArray(CTypeRef tp, size_t size) {
        return make<CArray>(tp.getName() + "[" + util::toString(size) + "]", getRef(tp), util::just(size));
    }

    CType::Ptr getInteger(const std::string& name, size_t bitsize, llvm::Signedness sign) {
        return make<CInteger>(name, bitsize, sign);
    }

    CType::Ptr getFloat(const std::string& name, size_t bitsize) {
        return make<CFloat>(name, bitsize);
    }

    // this overload is needed because you can get a pointer to undefined (yet) type
    CType::Ptr getPointer(const CTypeRef& ref) {
        return make<CPointer>(ref.getName() + "*", ref);
    }

    CType::Ptr getPointer(CType::Ptr ptr) {
        return getPointer(getRef(ptr));
    }

    CType::Ptr getStruct(const std::string& name, const std::vector<CStructMember>& members) {
        if(name == "") {
            auto generatedName =
                "{\n" +
                util::viewContainer(members)
                     .fold(std::string{}, LAM2(l, r, tfm::format("%s\n %s %s;", l, r.getType().getName(), r.getName()))) +
                "}";
            return make<CStruct>(generatedName, members, false);
        }
        return make<CStruct>(name, members, false);
    }

    CType::Ptr getOpaqueStruct(const std::string& name) {
        return make<CStruct>(name, std::vector<CStructMember>{}, true);
    }

    CType::Ptr getFunction(const CTypeRef& resultType, const std::vector<CTypeRef>& argumentTypes) {
        return make<CFunction>(
            resultType.getName() + "(" + util::viewContainer(argumentTypes).map(LAM(x, x.getName())).reduce("", LAM2(a, b, a + ", " + b)) + ")",
            resultType,
            argumentTypes
        );
    }

    CType::Ptr getVoid() {
        return make<CVoid>("void");
    }

private:
    CTypeRef processType(DIType meta, DebugInfoFinder& DFI, std::unordered_map<llvm::MDNode*, CTypeRef>& cache);

    struct QualTypeHash {
        size_t operator()(const clang::QualType& qt) const noexcept{
            return util::hash::simple_hash_value(qt.getTypePtr());
        }
    };

    CTypeRef processType(clang::QualType type, const clang::ASTContext& ctx, std::unordered_map<clang::QualType, CTypeRef, QualTypeHash>& cache);
public:
    void processTypes(DebugInfoFinder& DFI) {
        std::unordered_map<llvm::MDNode*, CTypeRef> cache;
        for(DIType dt : DFI.types()) if(dt) {
            if(DIMember(dt)) continue; // skip members
            processType(dt, DFI, cache);
        }
    }

    CType::Ptr getType(DIType meta, DebugInfoFinder& DFI) {
        std::unordered_map<llvm::MDNode*, CTypeRef> cache;
        return processType(meta, DFI, cache).get();
    }

    CType::Ptr getType(DIType meta, DebugInfoFinder& DFI, std::unordered_map<llvm::MDNode*, CTypeRef>& cache) {
        return processType(meta, DFI, cache).get();
    }

    std::unordered_map<clang::QualType, CTypeRef, QualTypeHash> cache;
    CType::Ptr getType(clang::QualType ast, const clang::ASTContext& ctx) {
        //std::unordered_map<clang::QualType, CTypeRef, QualTypeHash> cache;
        return processType(ast, ctx, cache).get();
    }

    CTypeContext::Ptr getCtx() const {
        return ctx;
    }

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* C_TYPE_FACTORY_H */
