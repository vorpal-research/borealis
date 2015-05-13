//
// Created by belyaev on 5/13/15.
//

#ifndef C_TYPE_FACTORY_H
#define C_TYPE_FACTORY_H

#include <queue>
#include <Util/functional.hpp>

#include "Codegen/llvm.h"
#include "Codegen/CType/CType.h"
#include "Codegen/CType/CTypeContext.h"

#include "Codegen/CType/CType.def"

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

private:
    CTypeRef processType(DIType meta, DebugInfoFinder& DFI) {
        if(auto array = DIArrayType(meta)) {
             auto elem = processType(DFI.resolve(array.getBaseType()), DFI);
             auto size = array.getArraySize();
             if(size == 0) return getRef(getArray(elem));
             else return getRef(getArray(elem, size));
        }

        if(auto pointer = DIDerivedType(meta)) if(pointer.getTag() == llvm::dwarf::DW_TAG_pointer_type) {
            auto elem = DFI.resolve(pointer.getTypeDerivedFrom());
            if(auto struct_ = DIStructType(elem)) {
                return getRef(struct_.getName());
            }
            auto deptr = processType(elem, DFI);
            return getRef(getPointer(deptr));
        }

        if(auto struct_ = DIStructType(meta)) {
            auto name = struct_->getName();
            auto elements = struct_.getMembers().asView().map(
                [&](DIMember member) {
                    auto memType = DFI.resolve(member.getType());
                    auto resolved = processType(memType, DFI);

                    return CStructMember(member.getOffsetInBits(), member.getName().str(), resolved);
                }
            ).toVector();
            return getRef(getStruct(name, elements));
        }

        if(auto alias = DIAlias(meta)) {
            auto elem = DFI.resolve(alias.getOriginal());
            auto processed = processType(elem, DFI);
            CQualifier qual;
            switch (alias.getTag()) {
            case llvm::dwarf::DW_TAG_typedef:
                return getRef(getTypedef(alias.getName().str(), processed));
            case llvm::dwarf::DW_TAG_const_type:
                return getRef(getConst(processed));
            case llvm::dwarf::DW_TAG_volatile_type:
                return getRef(getVolatile(processed));
            default: break;
            }
            UNREACHABLE("Unsupported alias type encountered");
        }

        if(auto basic = llvm::DIBasicType(meta)) if(basic.getTag() == llvm::dwarf::DW_TAG_base_type) {
            auto enc = basic.getEncoding();
            switch (enc) {
            case llvm::dwarf::DW_ATE_signed:
            case llvm::dwarf::DW_ATE_signed_char:
            case llvm::dwarf::DW_ATE_signed_fixed:
                return getRef(getInteger(basic.getName().str(), basic.getSizeInBits(), llvm::Signedness::Signed));
            case llvm::dwarf::DW_ATE_boolean:
            case llvm::dwarf::DW_ATE_unsigned:
            case llvm::dwarf::DW_ATE_unsigned_char:
            case llvm::dwarf::DW_ATE_unsigned_fixed:
                return getRef(getInteger(basic.getName().str(), basic.getSizeInBits(), llvm::Signedness::Unsigned));
            case llvm::dwarf::DW_ATE_float:
            case llvm::dwarf::DW_ATE_decimal_float:
                return getRef(getFloat(basic.getName().str(), basic.getSizeInBits()));
            }

            UNREACHABLE("Unsupported basic type encountered");
        }

        if(auto basic = DISubroutineType(meta)) {
            auto retTy = processType(DFI.resolve(basic.getReturnType()), DFI);
            auto pTypes = basic.getArgumentTypeView()
                               .map([&](llvm::DITypeRef ref) { return DFI.resolve(ref); })
                               .map(LAM(tp, processType(tp, DFI)))
                               .toVector();
            return getRef(getFunction(retTy, pTypes));
        }

        UNREACHABLE("Unsupported DIType encountered");
    }

public:
    void processTypes(DebugInfoFinder& DFI) {
        for(DIType dt : DFI.types()) if(dt) {
            processType(dt, DFI);
        }
    }


};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* C_TYPE_FACTORY_H */
