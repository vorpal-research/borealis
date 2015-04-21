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
#include <llvm/IR/Metadata.h>

#include "Util/util.h"
#include "Util/collections.hpp"

namespace borealis {

llvm::DebugLoc getFirstLocusForBlock(const llvm::BasicBlock* bb);
llvm::MDNode* getFirstMdNodeForBlock(const llvm::BasicBlock* bb, const char* id = "dbg");
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
void* MDNode2Ptr(const llvm::MDNode* ptr);

std::string getRawSource(const LocusRange&);

util::option<std::string> getAsCompileTimeString(const llvm::Value* value);

std::list<const llvm::Constant*> getAsSeqData(const llvm::Constant* value);

const llvm::TerminatorInst* getSingleReturnFor(const llvm::Instruction* i);

#include "Util/macros.h"
#include "Util/generate_macros.h"
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
        llvm::DIBasicType bTy(*this);
        if(!bTy.isBasicType()) return false;
        return bTy.getEncoding() == llvm::dwarf::DW_ATE_unsigned
            || bTy.getEncoding() == llvm::dwarf::DW_ATE_unsigned_char
            || bTy.getEncoding() == llvm::dwarf::DW_ATE_unsigned_fixed
            || bTy.getEncoding() == llvm::dwarf::DW_ATE_boolean;
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
        return getFieldAs<T>(Idx);
    }

    struct ElementAccessor {
        const DITypedArray* self;
        T operator()(unsigned ix) const { return self->getElement(ix); }
    };

    auto asView() const QUICK_RETURN(util::range(0U, this->getNumElements()).map(ElementAccessor{this}));
    auto begin() const QUICK_RETURN(this->asView().begin())
    auto end() const QUICK_RETURN(this->asView().end())

    // TODO: iterators?
};

STEAL_FROM_LLVM(DIDerivedType)
STEAL_FROM_LLVM_BEGIN(DICompositeType)
    bool Verify() const {
        return llvm::DICompositeType::Verify() && getTag() != llvm::dwarf::DW_TAG_array_type;
    }

    DITypedArray<DIType> getTypeArray() const  {
        return llvm::DICompositeType::getTypeArray();
    }
STEAL_FROM_LLVM_END()

struct DIArrayType : public llvm::DIDerivedType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIArrayType);
    DIArrayType(const llvm::MDNode* node): DIDerivedType(node) {
        if(this->getTag() != llvm::dwarf::DW_TAG_array_type) this->DbgNode = nullptr;
    };
    DIArrayType(llvm::DIDerivedType node): DIArrayType(static_cast<llvm::MDNode*>(node)) {};
    DIArrayType(const llvm::DIDescriptor& node): DIArrayType(static_cast<llvm::MDNode*>(node)) {};

    llvm::DITypeRef getBaseType() const {
        return getTypeDerivedFrom();
    }

    unsigned getArraySize() const {
        return getUnsignedField(5);
    }
};

struct DISubroutineType : public DICompositeType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DISubroutineType);
    DISubroutineType(const llvm::MDNode* node): DICompositeType(node) {
        if(this->getTag() != llvm::dwarf::DW_TAG_subroutine_type) this->DbgNode = nullptr;
    };
    DISubroutineType(DICompositeType node): DISubroutineType(static_cast<llvm::MDNode*>(node)) {};
    DISubroutineType(const llvm::DIDescriptor& node): DISubroutineType(static_cast<llvm::MDNode*>(node)) {};

    llvm::DITypeRef getReturnType() const {
        return getTypeArray().getElement(0);
    }

    auto getArgumentTypeView() const QUICK_RETURN(getTypeArray().asView().drop(1))
};

struct DIMember : public llvm::DIDerivedType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIMember);
    DIMember(const llvm::MDNode* node): llvm::DIDerivedType(node) {
        if(this->getTag() != llvm::dwarf::DW_TAG_member) this->DbgNode = nullptr;
    };
    DIMember(llvm::DIDerivedType node): DIMember(static_cast<llvm::MDNode*>(node)) {};
    DIMember(const llvm::DIDescriptor& node): DIMember(static_cast<llvm::MDNode*>(node)) {};

    llvm::DITypeRef getType() const {
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

    llvm::DITypeRef getOriginal() const {
        return getTypeDerivedFrom();
    }
};

struct DIStructType : public llvm::DICompositeType {
    DEFAULT_CONSTRUCTOR_AND_ASSIGN(DIStructType);
    DIStructType(const llvm::MDNode* node): llvm::DICompositeType(node) {
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

//std::set<DIType> flattenTypeTree(DIType di);
//std::map<llvm::Type*, DIType> flattenTypeTree(const std::pair<llvm::Type*, DIType>& tp);

#undef STEAL_FROM_LLVM
#undef STEAL_FROM_LLVM_END
#undef STEAL_FROM_LLVM_BEGIN

#include "Util/generate_unmacros.h"
#include "Util/unmacros.h"

/// DebugInfoFinder tries to list all debug info MDNodes used in a module. To
/// list debug info MDNodes used by an instruction, DebugInfoFinder uses
/// processDeclare, processValue and processLocation to handle DbgDeclareInst,
/// DbgValueInst and DbgLoc attached to instructions. processModule will go
/// through all DICompileUnits in llvm.dbg.cu and list debug info MDNodes
/// used by the CUs.
class DebugInfoFinder {
public:
    DebugInfoFinder() : TypeMapInitialized(false) {}

    /// processModule - Process entire module and collect debug info
    /// anchors.
    void processModule(const llvm::Module &M);

    /// processDeclare - Process DbgDeclareInst.
    void processDeclare(const llvm::Module &M, const llvm::DbgDeclareInst *DDI);
    /// Process DbgValueInst.
    void processValue(const llvm::Module &M, const llvm::DbgValueInst *DVI);
    /// processLocation - Process DILocation.
    void processLocation(const llvm::Module &M, llvm::DILocation Loc);

    /// Clear all lists.
    void reset();

private:
    /// Initialize TypeIdentifierMap.
    void InitializeTypeMap(const llvm::Module &M);

    /// processType - Process DIType.
    void processType(llvm::DIType DT);

    /// processSubprogram - Process DISubprogram.
    void processSubprogram(llvm::DISubprogram SP);

    void processScope(llvm::DIScope Scope);

    /// addCompileUnit - Add compile unit into CUs.
    bool addCompileUnit(llvm::DICompileUnit CU);

    /// addGlobalVariable - Add global variable into GVs.
    bool addGlobalVariable(llvm::DIGlobalVariable DIG);

    // addSubprogram - Add subprogram into SPs.
    bool addSubprogram(llvm::DISubprogram SP);

    /// addType - Add type into Tys.
    bool addType(llvm::DIType DT);

    bool addScope(llvm::DIScope Scope);

public:
    typedef llvm::SmallVectorImpl<llvm::DICompileUnit>::const_iterator compile_unit_iterator;
    typedef llvm::SmallVectorImpl<llvm::DISubprogram>::const_iterator subprogram_iterator;
    typedef llvm::SmallVectorImpl<llvm::DIGlobalVariable>::const_iterator global_variable_iterator;
    typedef llvm::SmallVectorImpl<llvm::DIType>::const_iterator type_iterator;
    typedef llvm::SmallVectorImpl<llvm::DIScope>::const_iterator scope_iterator;

    llvm::iterator_range<compile_unit_iterator> compile_units() const {
        return llvm::iterator_range<compile_unit_iterator>(CUs.begin(), CUs.end());
    }

    llvm::iterator_range<subprogram_iterator> subprograms() const {
        return llvm::iterator_range<subprogram_iterator>(SPs.begin(), SPs.end());
    }

    llvm::iterator_range<global_variable_iterator> global_variables() const {
        return llvm::iterator_range<global_variable_iterator>(GVs.begin(), GVs.end());
    }

    llvm::iterator_range<type_iterator> types() const {
        return llvm::iterator_range<type_iterator>(TYs.begin(), TYs.end());
    }

    llvm::iterator_range<scope_iterator> scopes() const {
        return llvm::iterator_range<scope_iterator>(Scopes.begin(), Scopes.end());
    }

    unsigned compile_unit_count() const { return CUs.size(); }
    unsigned global_variable_count() const { return GVs.size(); }
    unsigned subprogram_count() const { return SPs.size(); }
    unsigned type_count() const { return TYs.size(); }
    unsigned scope_count() const { return Scopes.size(); }

    template<class DI>
    DI resolve(llvm::DIRef<DI> ref) const {
        return ref.resolve(TypeIdentifierMap);
    }

private:
    llvm::SmallVector<llvm::DICompileUnit, 8> CUs;    // Compile Units
    llvm::SmallVector<llvm::DISubprogram, 8> SPs;    // Subprograms
    llvm::SmallVector<llvm::DIGlobalVariable, 8> GVs;    // Global Variables;
    llvm::SmallVector<llvm::DIType, 8> TYs;    // Types
    llvm::SmallVector<llvm::DIScope, 8> Scopes; // Scopes
    llvm::SmallPtrSet<llvm::MDNode *, 64> NodesSeen;
    llvm::DITypeIdentifierMap TypeIdentifierMap;
    /// Specify if TypeIdentifierMap is initialized.
    bool TypeMapInitialized;
};

std::set<DIType>& flattenTypeTree(const DebugInfoFinder& dfi, DIType di, std::set<DIType>& collected);
std::set<DIType> flattenTypeTree(const DebugInfoFinder& dfi,DIType di);
std::unordered_set<const llvm::Type*> flattenTypeTree(const llvm::Type* tp);
DIType stripAliases(const DebugInfoFinder& dfi, llvm::DITypeRef tp);
std::map<llvm::Type*, DIType>& flattenTypeTree(
    const DebugInfoFinder& dfi,
    const std::pair<llvm::Type*, DIType>& tp,
    std::map<llvm::Type*, DIType>& collected);
std::map<llvm::Type*, DIType> flattenTypeTree(
    const DebugInfoFinder& dfi,
    const std::pair<llvm::Type*, DIType>& tp);

namespace impl_ {

template<class AdditionalData>
class MagicVH : public llvm::CallbackVH {
public:
    MagicVH(llvm::Value* P, const AdditionalData& keep)
        : CallbackVH(P), keep(util::copy_or_share(keep)) { }

    MagicVH() = default;

private:
    std::shared_ptr<AdditionalData> keep;

public:
    virtual void deleted() override {
        llvm::CallbackVH::deleted();
        keep = nullptr;
    }

    virtual void allUsesReplacedWith(llvm::Value *) override {
        keep = nullptr;
    }

    std::shared_ptr<AdditionalData>& getData() {
        return keep;
    }

    const std::shared_ptr<AdditionalData>& getData() const{
        return keep;
    }

    bool empty() const {
        return getValPtr() == nullptr;
    }
};

} /* namespace impl_ */

template<class AdditionalData>
void addTracking(llvm::Value* v, const AdditionalData& ad) {
    using vh = impl_::MagicVH<AdditionalData>;
    static std::list<vh> cache;
    static size_t desiredCacheSize = 10;
    cache.push_back(vh(v, ad));
    if(cache.size() > desiredCacheSize) {
        for (auto it = cache.begin(); it != cache.end(); ++it) {
            if(it->empty()) it = cache.erase(it);
        }
        while(cache.size() > desiredCacheSize) {
            desiredCacheSize *= 2;
        }
    }
}

} // namespace borealis
namespace std {

template<class Data>
struct hash<borealis::impl_::MagicVH<Data>> {
    size_t operator()(const borealis::impl_::MagicVH<Data>& v) const noexcept {
        return borealis::util::hash::simple_hash_value((llvm::Value*)v, v.getData());
    }
};

} /* namespace std */

#endif /* CODEGEN_LLVM_H_ */
