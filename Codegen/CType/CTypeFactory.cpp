//
// Created by belyaev on 5/13/15.
//

#include "Codegen/CType/CTypeFactory.h"

#include "Util/macros.h"

namespace borealis {


CTypeRef CTypeFactory::processType(DIType meta, DebugInfoFinder& DFI) {

    if(!meta) {
        return getRef(getVoid());
    }

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
            .map([&](llvm::DITypeRef ref) {
                return DFI.resolve(ref);
            })
            .map(LAM(tp, processType(tp, DFI)))
            .toVector();
        return getRef(getFunction(retTy, pTypes));
    }

    UNREACHABLE("Unsupported DIType encountered");
}

} /* namespace borealis */

#include "Util/unmacros.h"
