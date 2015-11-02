//
// Created by belyaev on 5/13/15.
//

#include "Codegen/CType/CTypeFactory.h"

#include <clang/AST/Type.h>
#include <clang/AST/ASTContext.h>

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
            return getRef(getPointer(getRef(struct_.getName())));
        }
        auto deptr = processType(elem, DFI);
        return getRef(getPointer(deptr));
    }

    if(auto composite = DICompositeType(meta)) if(composite.getTag() == llvm::dwarf::DW_TAG_enumeration_type) {
        return getRef(getInteger(composite.getName().str(), composite.getSizeInBits(), llvm::Signedness::Unsigned));
    }

    if(auto struct_ = DIStructType(meta)) {
        auto name = struct_.getName();
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
            .map(LAM(tp, this->processType(tp, DFI)))
            .toVector();
        return getRef(getFunction(retTy, pTypes));
    }

    UNREACHABLE("Unsupported DIType encountered");
}

static llvm::StringRef getQTName(clang::QualType qtype) {
    if(auto&& iden = qtype.getBaseTypeIdentifier()) {
        return iden->getName();
    } else return "";
}

CTypeRef CTypeFactory::processType(clang::QualType qtype, const clang::ASTContext& ctx, std::unordered_map<clang::QualType, CTypeRef, QualTypeHash>& visited) {
    qtype = qtype.IgnoreParens();

    CTypeRef result{".$invalid_entry", this->ctx};

    ON_SCOPE_EXIT(visited.emplace(qtype, result));

    if(visited.count(qtype)) return result = visited.at(qtype);
    else if(qtype.getBaseTypeIdentifier()) {
        visited.emplace(qtype, getRef(qtype.getBaseTypeIdentifier()->getName().str()));
    }

    if(auto rt = clang::dyn_cast<clang::RecordType>(qtype)) {
        if(rt->getDecl()->getDefinition() == nullptr)
            return result = getRef(getOpaqueStruct(getQTName(qtype))); // it is an opaque decl
    }

    if(auto et = clang::dyn_cast<clang::ElaboratedType>(qtype)) {
        auto named = et->getNamedType();
        return result = processType(named, ctx, visited);
        // it is a forward decl
    }

    auto typeInfo = ctx.getTypeInfo(qtype);

    auto bitSize = typeInfo.first;

    auto qualified = qtype.hasLocalQualifiers();
    auto const_ = qtype.isLocalConstQualified();
    auto volatile_ = qtype.isLocalVolatileQualified();

    if(qualified) {
        auto baseType = processType(qtype.getUnqualifiedType(), ctx, visited);
        if (const_) baseType = getRef(getConst(getRef(baseType)));
        if (volatile_) baseType = getRef(getVolatile(getRef(baseType)));
        return result = baseType;
    }

    if(auto* tt = clang::dyn_cast<clang::TypedefType>(qtype)) {
        auto base = getRef(processType(qtype.getCanonicalType(), ctx, visited));
        return result = getRef(getTypedef(tt->getDecl()->getName().str(), base));
    }

    if(qtype->isVoidType()) {
        return result =  getRef(getVoid());
    }

    if(qtype->isIntegralOrEnumerationType() && qtype->isBuiltinType()) {
        auto bt = dyn_cast<clang::BuiltinType>(qtype);
        auto signedness = bt->isSignedIntegerOrEnumerationType() ? llvm::Signedness::Signed : llvm::Signedness::Unsigned;

        return result = getRef(getInteger(bt->getName(clang::PrintingPolicy(ctx.getLangOpts())), bitSize, signedness));
    }

    if(qtype->isIntegralOrEnumerationType() && qtype->isEnumeralType()) {
        auto bt = dyn_cast<clang::EnumType>(qtype);
        auto signedness = bt->isSignedIntegerOrEnumerationType() ? llvm::Signedness::Signed : llvm::Signedness::Unsigned;

        return result = getRef(getInteger(bt->getDecl()->getName(), bitSize, signedness));
    }

    if(qtype->isFloatingType()) {
        auto bt = clang::dyn_cast<clang::BuiltinType> (qtype);
        return result = getRef(getFloat(bt->getName(clang::PrintingPolicy(ctx.getLangOpts())), bitSize));
    }

    if(auto ptr = clang::dyn_cast<clang::PointerType>(qtype)) {
        auto base = processType(ptr->getPointeeType(), ctx, visited);
        return result = getRef(getPointer(base));
    }

    if(auto arr = clang::dyn_cast<clang::ConstantArrayType>(qtype)) {
        auto base = processType(arr->getElementType(), ctx, visited);
        return result = getRef(getArray(base, arr->getSize().getLimitedValue()));
    }

    // all ArrayTypes excluding ConstantArrayType
    if(auto arr = clang::dyn_cast<clang::ArrayType>(qtype)) {
        auto base = processType(arr->getElementType(), ctx, visited);
        return result = getRef(getArray(base));
    }

    if(auto rec = clang::dyn_cast<clang::RecordType>(qtype)) {
        auto decl = rec->getDecl()->getDefinition();
        auto name = decl->getName().str();

        auto elements = util::view(decl->field_begin(), decl->field_end())
                       .map(
                           [&](clang::FieldDecl* fd) -> CStructMember {
                               auto type = processType(fd->getType(), ctx, visited);
                               auto name = fd->getName().str();
                               auto offset = ctx.getFieldOffset(fd);
                               return CStructMember(offset, name, type);
                           }
                       )
                       .toVector();
        return result = getRef(getStruct(name, elements));
    }

    if(auto ft = clang::dyn_cast<clang::FunctionProtoType>(qtype)) {
        return result = getRef(getFunction(
            processType(ft->getReturnType(), ctx, visited),
            util::viewContainer(ft->getParamTypes())
                 .toVectorView()
                 .map(LAM(qt, this->processType(qt, ctx, visited)))
                 .toVector()
        ));
    }

    if(auto ft = clang::dyn_cast<clang::FunctionNoProtoType>(qtype)) {
        return result = getRef(getFunction(
            processType(ft->getReturnType(), ctx, visited),
            {}
        ));
    }

    if(auto at = clang::dyn_cast<clang::AdjustedType>(qtype)) {
        processType(at->getOriginalType(), ctx, visited);
        return result = processType(at->getAdjustedType(), ctx, visited);
    }

    UNREACHABLE("Unsupported QualType encountered");
}
} /* namespace borealis */

#include "Util/unmacros.h"
