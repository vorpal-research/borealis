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

llvm::DebugLoc getFirstLocusForBlock(llvm::BasicBlock* bb);
llvm::MDNode* getFirstMdNodeForBlock(llvm::BasicBlock* bb, const char* id = "dbg");
void approximateAllDebugLocs(llvm::BasicBlock* bb);

void insertBeforeWithLocus(
        llvm::Instruction* what,
        llvm::Instruction* before,
        const Locus& loc);
void setDebugLocusWithCopiedScope(
        llvm::Instruction* to,
        llvm::Instruction* from,
        const Locus& loc);

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, const void* ptr);
void* MDNode2Ptr(llvm::MDNode* ptr);

std::string getRawSource(const LocusRange&);

util::option<std::string> getAsCompileTimeString(llvm::Value* value);

std::list<llvm::Constant*> getAsSeqData(llvm::Constant* value);



#include "Util/macros.h"
#define STEAL_FROM_LLVM_BEGIN(NAME) \
    struct NAME : public llvm::NAME { \
        DEFAULT_CONSTRUCTOR_AND_ASSIGN(NAME); \
        NAME(const llvm::MDNode* node): llvm::NAME(node) { \
            if(!this->Verify()) this->DbgNode = nullptr; \
        } \
        NAME(llvm::NAME node): NAME(static_cast<llvm::MDNode*>(node)) {}; \
        NAME(const llvm::DIDescriptor& node): NAME(static_cast<llvm::MDNode*>(node)) {}; \

#define STEAL_FROM_LLVM_END() };

#define STEAL_FROM_LLVM(NAME) STEAL_FROM_LLVM_BEGIN(NAME) STEAL_FROM_LLVM_END()

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



template<class T, unsigned Tag = ~0U>
struct DITypedArray : public llvm::DIArray {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DITypedArray);
    DITypedArray(const llvm::MDNode* node): llvm::DIArray(node) {};
    DITypedArray(llvm::DIArray node): DITypedArray(static_cast<llvm::MDNode*>(node)) {};
    DITypedArray(const llvm::DIDescriptor& node): DITypedArray(static_cast<llvm::MDNode*>(node)) {};

    T getElement(unsigned Idx) const {
        if(Tag != ~0U && this->getTag() != Tag) return nullptr;
        return T{ getDescriptorField(Idx) };
    }

    // TODO: iterators?
};

STEAL_FROM_LLVM(DIDerivedType)
STEAL_FROM_LLVM(DICompositeType)

struct DIMember : public llvm::DIDerivedType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIMember);
    DIMember(const llvm::MDNode* node): llvm::DIDerivedType(node) {
        if(this->getTag() != llvm::dwarf::DW_TAG_member) this->DbgNode = nullptr;
    };
    DIMember(llvm::DIDerivedType node): DIMember(static_cast<llvm::MDNode*>(node)) {};
    DIMember(const llvm::DIDescriptor& node): DIMember(static_cast<llvm::MDNode*>(node)) {};

    llvm::DIType getType() const {
        return getTypeDerivedFrom();
    }
};

// A type alias maps to the same llvm type as its argument
struct DIAlias : public llvm::DIDerivedType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIAlias);
    DIAlias(const llvm::MDNode* node): llvm::DIDerivedType(node) {
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
    DIStructType(const llvm::MDNode* node): llvm::DICompositeType(node) {
        while(DIAlias(this->DbgNode)) {
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

struct DIClangGlobalDesc : public llvm::DIDescriptor {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIClangGlobalDesc);
    DIClangGlobalDesc(const llvm::MDNode* node): llvm::DIDescriptor(node) {
        if( node->getNumOperands() != 2
        || !llvm::isa<llvm::GlobalValue>(node->getOperand(0))
        || !llvm::isa<llvm::ConstantInt>(node->getOperand(1))
        ){
            this->DbgNode = nullptr;
        }
    }
    DIClangGlobalDesc(const llvm::DIDescriptor& node): DIClangGlobalDesc(static_cast<llvm::MDNode*>(node)) {};

    llvm::GlobalValue* getGlobal() const {
        if(!DbgNode) return nullptr;
        return llvm::dyn_cast_or_null<llvm::GlobalValue>(DbgNode->getOperand(0));
    }
    clang::VarDecl* getClangVarDecl() const {
        if(!DbgNode) return nullptr;
        auto field = getUInt64Field(1);
        // this is generally fucked up, but IS the way they do it in clang
        return static_cast<clang::VarDecl*>(reinterpret_cast<void*>(field));
    }
};

struct DIBorealisVarDesc : public llvm::DIDescriptor {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIBorealisVarDesc);
    DIBorealisVarDesc(const llvm::MDNode* node): llvm::DIDescriptor(node) {
        if( node->getNumOperands() != 5
        || !llvm::isa<llvm::GlobalValue>(node->getOperand(0))
        || !llvm::isa<llvm::MDString>   (node->getOperand(1))
        || !llvm::isa<llvm::ConstantInt>(node->getOperand(2))
        || !llvm::isa<llvm::ConstantInt>(node->getOperand(3))
        || !llvm::isa<llvm::MDString>   (node->getOperand(4))
        ){
            this->DbgNode = nullptr;
        }
    }
    DIBorealisVarDesc(const llvm::DIDescriptor& node): DIBorealisVarDesc(static_cast<llvm::MDNode*>(node)) {};

    llvm::GlobalValue* getGlobal() const {
        if(!DbgNode) return nullptr;
        return llvm::dyn_cast_or_null<llvm::GlobalValue>(DbgNode->getOperand(0));
    }
    llvm::StringRef getVarName() const {
        return llvm::cast<llvm::MDString>(DbgNode->getOperand(1))->getString();
    }

    unsigned long long getLine() const {
        return getUInt64Field(3);
    }

    unsigned long long getCol() const {
        return getUInt64Field(2);
    }

    llvm::StringRef getFileName() const {
        return llvm::cast<llvm::MDString>(DbgNode->getOperand(4))->getString();
    }
};

DIType stripAliases(DIType tp);
std::set<DIType> flattenTypeTree(DIType di);
std::map<llvm::Type*, DIType> flattenTypeTree(const std::pair<llvm::Type*, DIType>& tp);

#undef STEAL_FROM_LLVM
#undef STEAL_FROM_LLVM_END
#undef STEAL_FROM_LLVM_BEGIN

#include "Util/unmacros.h"

} // namespace borealis

#endif /* CODEGEN_LLVM_H_ */
