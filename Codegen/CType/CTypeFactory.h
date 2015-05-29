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

#include "Util/functional.hpp"

#include "Util/macros.h"

namespace borealis {

class CTypeFactory {

    CTypeContext::Ptr ctx;

    CType::Ptr record(CType::Ptr tp) {
        ctx->put(tp);
        return tp;
    }

    CTypeRef getRef(const std::string& name) {
        return CTypeRef(name, ctx);
    }

    CTypeRef getRef(CTypeRef ptr) {
        return ptr;
    }

    template<class T, class ...Args>
    CType::Ptr make_shared(Args&&... args) {
        return std::shared_ptr<T>( new T(std::forward<Args>(args)...) );
    }

public:
    CTypeFactory() : ctx(std::make_shared<CTypeContext>()) {}

    CTypeRef getRef(CType::Ptr tp) {
        ctx->put(tp);
        return CTypeRef(tp->getName(), ctx);
    }

    CType::Ptr getTypedef(const std::string& name, CTypeRef tp) {
        return record(make_shared<CAlias>(name, getRef(tp), CQualifier::TYPEDEF));
    }

    CType::Ptr getConst(CTypeRef tp) {
        return record(make_shared<CAlias>("const " + tp->getName(), getRef(tp), CQualifier::CONST));
    }

    CType::Ptr getVolatile(CTypeRef tp) {
        return record(make_shared<CAlias>("volatile " + tp->getName(), getRef(tp), CQualifier::VOLATILE));
    }

    CType::Ptr getArray(CTypeRef tp) {
        return record(make_shared<CArray>(tp->getName() + "[]", getRef(tp), util::nothing()));
    }

    CType::Ptr getArray(CTypeRef tp, size_t size) {
        return record(make_shared<CArray>(tp->getName() + "[" + util::toString(size) + "]", getRef(tp), util::just(size)));
    }

    CType::Ptr getInteger(const std::string& name, size_t bitsize, llvm::Signedness sign) {
        return record(make_shared<CInteger>(name, bitsize, sign));
    }

    CType::Ptr getFloat(const std::string& name, size_t bitsize) {
        return record(make_shared<CFloat>(name, bitsize));
    }

    // this overload is needed because you can get a pointer to undefined (yet) type
    CType::Ptr getPointer(const CTypeRef& ref) {
        return record(make_shared<CPointer>(ref.getName() + "*", ref));
    }

    CType::Ptr getStruct(const std::string& name, const std::vector<CStructMember>& members) {
        return record(make_shared<CStruct>(name, members));
    }

    CType::Ptr getFunction(const CTypeRef& resultType, const std::vector<CTypeRef>& argumentTypes) {
        return make_shared<CFunction>(
            resultType.getName() + "(" + util::viewContainer(argumentTypes).map(LAM(x, x.getName())).reduce("", LAM2(a, b, a + ", " + b)) + ")",
            resultType,
            argumentTypes
        );
    }

    CType::Ptr getVoid() {
        return make_shared<CVoid>("void");
    }

private:
    CTypeRef processType(DIType meta, DebugInfoFinder& DFI);

public:
    void processTypes(DebugInfoFinder& DFI) {
        for(DIType dt : DFI.types()) if(dt) {
            if(DIMember(dt)) continue; // skip members
            processType(dt, DFI);
        }
    }


};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* C_TYPE_FACTORY_H */
