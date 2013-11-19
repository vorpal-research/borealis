/*
 * llvm.h
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#ifndef CODEGEN_LLVM_H_
#define CODEGEN_LLVM_H_

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Metadata.h>

#include "Util/util.h"

namespace borealis {

void insertBeforeWithLocus(
        llvm::Instruction* what,
        llvm::Instruction* before,
        const Locus& loc);
void setDebugLocusWithCopiedScope(
        llvm::Instruction* to,
        llvm::Instruction* from,
        const Locus& loc);

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, void* ptr);
void* MDNode2Ptr(llvm::MDNode* ptr);

std::string getRawSource(const clang::FileManager& sm, const LocusRange& range);

unsigned long long getTypeSizeInElems(llvm::Type* type);

util::option<std::string> getAsCompileTimeString(llvm::Value* value);

#include "Util/macros.h"
#define STEAL_FROM_LLVM_BEGIN(NAME) \
    struct NAME : public llvm::NAME { \
        DEFAULT_CONSTRUCTOR_AND_ASSIGN(NAME); \
        NAME(llvm::MDNode* node): llvm::NAME(node) { \
            if(!this->Verify()) this->DbgNode = nullptr; \
        } \
        NAME(llvm::NAME node): NAME(static_cast<llvm::MDNode*>(node)) {}; \
        NAME(const llvm::DIDescriptor& node): NAME(static_cast<llvm::MDNode*>(node)) {}; \

#define STEAL_FROM_LLVM_END() };

STEAL_FROM_LLVM_BEGIN(DIType)
    bool isUnsignedDIType() const {
        // this is generally fucked up
        return static_cast<llvm::DIType*>(const_cast<DIType*>(this))->isUnsignedDIType();
    }

    llvm::Signedness getSignedness() const {
        return !isValid() ? llvm::Signedness::Unknown :
               isUnsignedDIType() ? llvm::Signedness::Unsigned :
               llvm::Signedness::Signed;
    }
STEAL_FROM_LLVM_END()

#define STEAL_FROM_LLVM(NAME) STEAL_FROM_LLVM_BEGIN(NAME) STEAL_FROM_LLVM_END()


template<class T, unsigned Tag = ~0U>
struct DITypedArray: public llvm::DIArray {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DITypedArray);
    DITypedArray(llvm::MDNode* node): llvm::DIArray(node) {};
    DITypedArray(llvm::DIArray node): DITypedArray(static_cast<llvm::MDNode*>(node)) {};
    DITypedArray(const llvm::DIDescriptor& node): DITypedArray(static_cast<llvm::MDNode*>(node)) {};

    T getElement(unsigned Idx) const {
        auto descr = getDescriptorField(Idx);
        if(Tag != ~0U && this->getTag() != Tag) return nullptr;
        return T{ descr };
    }

    // TODO: iterators?
};

STEAL_FROM_LLVM(DIDerivedType)
STEAL_FROM_LLVM(DICompositeType)

struct DIMember : public llvm::DIDerivedType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIMember);
    DIMember(llvm::MDNode* node): llvm::DIDerivedType(node) {
        if(this->getTag() != llvm::dwarf::DW_TAG_member) this->DbgNode = nullptr;
    };
    DIMember(llvm::DIDerivedType node): DIMember(static_cast<llvm::MDNode*>(node)) {};
    DIMember(const llvm::DIDescriptor& node): DIMember(static_cast<llvm::MDNode*>(node)) {};

    llvm::DIType getType() const {
        return getTypeDerivedFrom();
    }

};

// any type that maps to the same llvm type as its argument
struct DIAlias : public llvm::DIDerivedType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIAlias);
    DIAlias(llvm::MDNode* node): llvm::DIDerivedType(node) {
        switch(this->getTag()) {
        case llvm::dwarf::DW_TAG_typedef:
        case llvm::dwarf::DW_TAG_member:
        case llvm::dwarf::DW_TAG_const_type:
        case llvm::dwarf::DW_TAG_volatile_type:
        case llvm::dwarf::DW_TAG_restrict_type:
        case llvm::dwarf::DW_TAG_friend:
            break;
        default:
            this->DbgNode = nullptr;
            break;
        }
    };
    DIAlias(llvm::DIDerivedType node): DIAlias(static_cast<llvm::MDNode*>(node)) {};
    DIAlias(const llvm::DIDescriptor& node): DIAlias(static_cast<llvm::MDNode*>(node)) {};

    llvm::DIType getOriginal() const {
        return getTypeDerivedFrom();
    }
};

struct DIStructType : public llvm::DICompositeType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIStructType);
    DIStructType(llvm::MDNode* node): llvm::DICompositeType(node) {
        while(this->getTag() == llvm::dwarf::DW_TAG_typedef) {
            this->DbgNode = this->getTypeDerivedFrom();
        }
        if(this->getTag() != llvm::dwarf::DW_TAG_structure_type) this->DbgNode = nullptr;
    };
    DIStructType(llvm::DICompositeType node): DIStructType(static_cast<llvm::MDNode*>(node)) {};
    DIStructType(const llvm::DIDescriptor& node): DIStructType(static_cast<llvm::MDNode*>(node)) {};

    DITypedArray<DIMember> getMembers() const {
        return DITypedArray<DIMember>{ getTypeArray() };
    }

};

DIType stripAliases(DIType tp);
std::set<DIType> flattenTypeTree(DIType di);
std::map<llvm::Type*, DIType> flattenTypeTree(const std::pair<llvm::Type*, DIType>& tp);

#undef STEAL_FROM_LLVM
#undef STEAL_FROM_LLVM_BEGIN
#undef STEAL_FROM_LLVM_END

#include "Util/unmacros.h"

} // namespace borealis

#endif /* CODEGEN_LLVM_H_ */
