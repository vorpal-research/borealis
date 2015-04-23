/*
 * llvm.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Type.h>

#include "Codegen/FileManager.h"
#include "Codegen/llvm.h"
#include "Util/functional.hpp"

#include "Util/macros.h"
namespace borealis {

llvm::DebugLoc getFirstLocusForBlock(const llvm::BasicBlock* bb) {
    for(auto& I : *bb) {
        const auto& dl = I.getDebugLoc();
        if(dl.isUnknown()) continue;

        return dl;
    }

    return llvm::DebugLoc{};
}

llvm::MDNode* getFirstMdNodeForBlock(const llvm::BasicBlock* bb, const char* id) {
    for(auto& I : *bb) {
        auto* md = I.getMetadata(id);
        if(!md) continue;

        return md;
    }

    return nullptr;
}

template<class It>
static std::reverse_iterator<It> reverse(It it) {
    return std::reverse_iterator<It>(it);
}

void approximateAllDebugLocs(llvm::BasicBlock* bb) {
    using namespace llvm;

    DebugLoc ldl;
    MDNode* lmd = nullptr;
    auto vec = util::viewContainer(*bb).map(ops::take_pointer).toVector();

    for(auto* pI: util::view(vec.rbegin(), vec.rend())) {
        auto& I = *pI;
        auto dl = I.getDebugLoc();
        if( ! dl.isUnknown()) ldl = dl;
        else I.setDebugLoc(ldl);
        auto md = I.getMetadata("dbg");
        if(md) lmd = md;
        else I.setMetadata("dbg", lmd);
    }
    return;

}

void insertBeforeWithLocus(
        llvm::Instruction* what,
        llvm::Instruction* before,
        const Locus& loc) {
    what->insertBefore(before);
    setDebugLocusWithCopiedScope(what, before, loc);
}

void insertAfterWithLocus(
        llvm::Instruction* what,
        llvm::Instruction* after,
        const Locus& loc) {
    what->insertAfter(after);
    setDebugLocusWithCopiedScope(what, after, loc);
}

void setDebugLocusWithCopiedScope(
        llvm::Instruction* to,
        llvm::Instruction* from,
        const Locus& loc) {
    using namespace llvm;

    auto& ctx = from->getContext();

    auto* dbg = MDNode::get(
            ctx,
            std::vector<Value*>{
                    ConstantInt::get(Type::getInt32Ty(ctx), loc.loc.line),
                    ConstantInt::get(Type::getInt32Ty(ctx), loc.loc.col),
                    from->getDebugLoc().getScope(ctx),
                    nullptr
            });
    to->setMetadata("dbg", dbg);
}

llvm::MDNode* ptr2MDNode(llvm::LLVMContext& ctx, const void* ptr) {
    llvm::Constant* ptrAsInt = llvm::ConstantInt::get(
        ctx,
        llvm::APInt(sizeof(uintptr_t) * 8, reinterpret_cast<uintptr_t>(ptr))
    );
    return llvm::MDNode::get(ctx, ptrAsInt);
}

void* MDNode2Ptr(const llvm::MDNode* ptr) {
    if (ptr)
        if (auto* i = llvm::dyn_cast<llvm::ConstantInt>(ptr->getOperand(0)))
            return reinterpret_cast<void*>(static_cast<uintptr_t>(i->getLimitedValue()));
    return nullptr;
}

std::string getRawSource(const LocusRange& lr) {
    static FileManager fm;
    return fm.read(lr);
}

util::option<std::string> getAsCompileTimeString(const llvm::Value* value) {
    using namespace llvm;

    auto* v = value->stripPointerCasts();
    if (auto* gv = dyn_cast<GlobalVariable>(v)) {
        if (gv->hasInitializer()) v = gv->getInitializer();
    }

    if (auto* cv = dyn_cast<ConstantDataSequential>(v)) {
        if (cv->isCString()) return util::just(cv->getAsCString().str());
        else if (cv->isString()) return util::just(cv->getAsString().str());
    }

    return util::nothing();
}

std::list<const llvm::Constant*> getAsSeqData(const llvm::Constant* value) {
    std::list<const llvm::Constant*> res;

    llvm::Type* type = value->getType();

    if (type->isSingleValueType()) {
        res.push_back(value);

    } else if (llvm::ArrayType* arrType = llvm::dyn_cast<llvm::ArrayType>(type)) {
        auto nested = util::range(0UL, arrType->getArrayNumElements())
            .map(APPLY(value->getAggregateElement))
            .map(APPLY(getAsSeqData))
            .toList();
        res = util::viewContainer(nested).flatten().toList();

    } else if (llvm::StructType* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        auto nested = util::range(0U, structType->getStructNumElements())
            .map(APPLY(value->getAggregateElement))
            .map(APPLY(getAsSeqData))
            .toList();
        res = util::viewContainer(nested).flatten().toList();

    } else BYE_BYE(decltype(res), "Unsupported constant-as-seq-data type: " + util::toString(type));

    return std::move(res);
}

std::set<DIType>& flattenTypeTree(const DebugInfoFinder& dfi, DIType di, std::set<DIType>& collected) {
    if(collected.count(di)) return collected;
    collected.insert(di);

    if(DICompositeType struct_ = di) {
        auto members = struct_.getTypeArray();
        for(auto i = 0U; i < members.getNumElements(); ++i) {
            auto mem = members.getElement(i);
            flattenTypeTree(dfi, mem, collected);
        }
    } else if(DIDerivedType derived = di) {
        flattenTypeTree(dfi, dfi.resolve(derived.getTypeDerivedFrom()), collected);
    }

    return collected;
}

std::set<DIType> flattenTypeTree(const DebugInfoFinder& dfi,DIType di) {
    std::set<DIType> collected;
    flattenTypeTree(dfi, di, collected);
    return std::move(collected);
}

DIType stripAliases(const DebugInfoFinder& dfi, llvm::DITypeRef tref) {
    auto tp = dfi.resolve(tref);
    if(DIAlias alias = tp)
        return stripAliases(dfi, alias.getOriginal());
    else return tp;
}

std::map<llvm::Type*, DIType>& flattenTypeTree(
    const DebugInfoFinder& dfi,
    const std::pair<llvm::Type*, DIType>& tp,
    std::map<llvm::Type*, DIType>& collected) {

    auto* type = tp.first;

    if(!tp.second) return collected;

    auto di = stripAliases(dfi, tp.second);

    if(collected.count(type)) return collected;
    collected.insert({type, di});

    if(DIArrayType arr_ = di) {
        auto member = stripAliases(dfi, arr_.getBaseType());
        ASSERTC(tp.first->isArrayTy());
        auto llmember = tp.first->getArrayElementType();
        flattenTypeTree(dfi, {llmember, member}, collected);

    } else if(DICompositeType struct_ = di) {
        auto members = struct_.getTypeArray();
        ASSERTC(members.getNumElements() == type->getNumContainedTypes());
        for(auto i = 0U; i < members.getNumElements(); ++i) {
            auto mem = type->getContainedType(i);
            auto mmem = members.getElement(i);

            if (!mmem){
                ASSERTC(mem->isVoidTy()); // typical for functions returning void
            } else {
                flattenTypeTree(dfi, {mem, mmem}, collected);
            }
        }
    } else if(DIDerivedType derived = di) {
        flattenTypeTree(dfi, {type->getContainedType(0), dfi.resolve(derived.getTypeDerivedFrom())}, collected);
    }

    return collected;
}

std::map<llvm::Type*, DIType> flattenTypeTree(const DebugInfoFinder& dfi,
    const std::pair<llvm::Type*, DIType>& tp) {
    std::map<llvm::Type*, DIType> collected;
    flattenTypeTree(dfi, tp, collected);
    return std::move(collected);
}

std::unordered_set<const llvm::Type*>& flattenTypeTree(
        const llvm::Type* tp,
        std::unordered_set<const llvm::Type*>& collected
    ) {
    if(collected.count(tp)) return collected;

    collected.insert(tp);
    if(auto&& str = llvm::dyn_cast<llvm::StructType>(tp)) {
        for(auto&& el : borealis::util::view(str->element_begin(), str->element_end())) {
            flattenTypeTree(el, collected);
        }
    }
    return collected;
}

std::unordered_set<const llvm::Type*> flattenTypeTree(const llvm::Type* tp) {
    std::unordered_set<const llvm::Type*> ret;
    flattenTypeTree(tp, ret);
    return std::move(ret);
}

//===----------------------------------------------------------------------===//
// DebugInfoFinder implementations.
//===----------------------------------------------------------------------===//

void DebugInfoFinder::reset() {
    CUs.clear();
    SPs.clear();
    GVs.clear();
    TYs.clear();
    Scopes.clear();
    NodesSeen.clear();
    TypeIdentifierMap.clear();
    TypeMapInitialized = false;
}

void DebugInfoFinder::InitializeTypeMap(const llvm::Module &M) {
    if (!TypeMapInitialized)
        if (llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu")) {
            TypeIdentifierMap = generateDITypeIdentifierMap(CU_Nodes);
            TypeMapInitialized = true;
        }
}

/// processModule - Process entire module and collect debug info.
void DebugInfoFinder::processModule(const llvm::Module &M) {
    InitializeTypeMap(M);
    if (llvm::NamedMDNode *CU_Nodes = M.getNamedMetadata("llvm.dbg.cu")) {
        for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
            llvm::DICompileUnit CU(CU_Nodes->getOperand(i));
            addCompileUnit(CU);
            llvm::DIArray GVs = CU.getGlobalVariables();
            for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
                llvm::DIGlobalVariable DIG(GVs.getElement(i));
                if (addGlobalVariable(DIG)) {
                    processScope(DIG.getContext());
                    processType(DIG.getType().resolve(TypeIdentifierMap));
                }
            }
            llvm::DIArray SPs = CU.getSubprograms();
            for (unsigned i = 0, e = SPs.getNumElements(); i != e; ++i)
                processSubprogram(llvm::DISubprogram(SPs.getElement(i)));
            llvm::DIArray EnumTypes = CU.getEnumTypes();
            for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i)
                processType(llvm::DIType(EnumTypes.getElement(i)));
            llvm::DIArray RetainedTypes = CU.getRetainedTypes();
            for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i)
                processType(llvm::DIType(RetainedTypes.getElement(i)));
            llvm::DIArray Imports = CU.getImportedEntities();
            for (unsigned i = 0, e = Imports.getNumElements(); i != e; ++i) {
                llvm::DIImportedEntity Import = llvm::DIImportedEntity(Imports.getElement(i));
                llvm::DIDescriptor Entity = Import.getEntity().resolve(TypeIdentifierMap);
                if (Entity.isType())
                    processType(llvm::DIType(Entity));
                else if (Entity.isSubprogram())
                    processSubprogram(llvm::DISubprogram(Entity));
                else if (Entity.isNameSpace())
                    processScope(llvm::DINameSpace(Entity).getContext());
            }
        }
    }
}

/// processLocation - Process DILocation.
void DebugInfoFinder::processLocation(const llvm::Module &M, llvm::DILocation Loc) {
    if (!Loc)
        return;
    InitializeTypeMap(M);
    processScope(Loc.getScope());
    processLocation(M, Loc.getOrigLocation());
}

/// processType - Process DIType.
void DebugInfoFinder::processType(llvm::DIType DT) {
    if (!addType(DT))
        return;
    processScope(DT.getContext().resolve(TypeIdentifierMap));
    if (DT.isCompositeType()) {
        llvm::DICompositeType DCT(DT);
        processType(DCT.getTypeDerivedFrom().resolve(TypeIdentifierMap));
        llvm::DIArray DA = DCT.getTypeArray();
        for (unsigned i = 0, e = DA.getNumElements(); i != e; ++i) {
            llvm::DIDescriptor D = DA.getElement(i);
            if (D.isType())
                processType(llvm::DIType(D));
            else if (D.isSubprogram())
                processSubprogram(llvm::DISubprogram(D));
        }
    } else if (DT.isDerivedType()) {
        llvm::DIDerivedType DDT(DT);
        processType(DDT.getTypeDerivedFrom().resolve(TypeIdentifierMap));
    }
}

void DebugInfoFinder::processScope(llvm::DIScope Scope) {
    if (Scope.isType()) {
        llvm::DIType Ty(Scope);
        processType(Ty);
        return;
    }
    if (Scope.isCompileUnit()) {
        addCompileUnit(llvm::DICompileUnit(Scope));
        return;
    }
    if (Scope.isSubprogram()) {
        processSubprogram(llvm::DISubprogram(Scope));
        return;
    }
    if (!addScope(Scope))
        return;
    if (Scope.isLexicalBlock()) {
        llvm::DILexicalBlock LB(Scope);
        processScope(LB.getContext());
    } else if (Scope.isLexicalBlockFile()) {
        llvm::DILexicalBlockFile LBF = llvm::DILexicalBlockFile(Scope);
        processScope(LBF.getScope());
    } else if (Scope.isNameSpace()) {
        llvm::DINameSpace NS(Scope);
        processScope(NS.getContext());
    }
}

/// processSubprogram - Process DISubprogram.
void DebugInfoFinder::processSubprogram(llvm::DISubprogram SP) {
    if (!addSubprogram(SP))
        return;
    processScope(SP.getContext().resolve(TypeIdentifierMap));
    processType(SP.getType());
    llvm::DIArray TParams = SP.getTemplateParams();
    for (unsigned I = 0, E = TParams.getNumElements(); I != E; ++I) {
        llvm::DIDescriptor Element = TParams.getElement(I);
        if (Element.isTemplateTypeParameter()) {
            llvm::DITemplateTypeParameter TType(Element);
            processScope(TType.getContext().resolve(TypeIdentifierMap));
            processType(TType.getType().resolve(TypeIdentifierMap));
        } else if (Element.isTemplateValueParameter()) {
            llvm::DITemplateValueParameter TVal(Element);
            processScope(TVal.getContext().resolve(TypeIdentifierMap));
            processType(TVal.getType().resolve(TypeIdentifierMap));
        }
    }
}

/// processDeclare - Process DbgDeclareInst.
void DebugInfoFinder::processDeclare(const llvm::Module &M,
    const llvm::DbgDeclareInst *DDI) {
    llvm::MDNode *N = llvm::dyn_cast<llvm::MDNode>(DDI->getVariable());
    if (!N)
        return;
    InitializeTypeMap(M);

    llvm::DIDescriptor DV(N);
    if (!DV.isVariable())
        return;

    if (!NodesSeen.insert(DV))
        return;
    processScope(llvm::DIVariable(N).getContext());
    processType(llvm::DIVariable(N).getType().resolve(TypeIdentifierMap));
}

void DebugInfoFinder::processValue(const llvm::Module &M, const llvm::DbgValueInst *DVI) {
    llvm::MDNode *N = llvm::dyn_cast<llvm::MDNode>(DVI->getVariable());
    if (!N)
        return;
    InitializeTypeMap(M);

    llvm::DIDescriptor DV(N);
    if (!DV.isVariable())
        return;

    if (!NodesSeen.insert(DV))
        return;
    processScope(llvm::DIVariable(N).getContext());
    processType(llvm::DIVariable(N).getType().resolve(TypeIdentifierMap));
}

/// addType - Add type into Tys.
bool DebugInfoFinder::addType(llvm::DIType DT) {
    if (!DT)
        return false;

    if (!NodesSeen.insert(DT))
        return false;

    TYs.push_back(DT);
    return true;
}

/// addCompileUnit - Add compile unit into CUs.
bool DebugInfoFinder::addCompileUnit(llvm::DICompileUnit CU) {
    if (!CU)
        return false;
    if (!NodesSeen.insert(CU))
        return false;

    CUs.push_back(CU);
    return true;
}

/// addGlobalVariable - Add global variable into GVs.
bool DebugInfoFinder::addGlobalVariable(llvm::DIGlobalVariable DIG) {
    if (!DIG)
        return false;

    if (!NodesSeen.insert(DIG))
        return false;

    GVs.push_back(DIG);
    return true;
}

// addSubprogram - Add subprgoram into SPs.
bool DebugInfoFinder::addSubprogram(llvm::DISubprogram SP) {
    if (!SP)
        return false;

    if (!NodesSeen.insert(SP))
        return false;

    SPs.push_back(SP);
    return true;
}

bool DebugInfoFinder::addScope(llvm::DIScope Scope) {
    if (!Scope)
        return false;
    // FIXME: Ocaml binding generates a scope with no content, we treat it
    // as null for now.
    if (Scope->getNumOperands() == 0)
        return false;
    if (!NodesSeen.insert(Scope))
        return false;
    Scopes.push_back(Scope);
    return true;
}


const llvm::TerminatorInst* getSingleReturnFor(const llvm::Instruction* i) {
    auto bb = i->getParent();
    auto bbTerm = bb->getTerminator();
    while(bbTerm) {
        if (bbTerm->getNumSuccessors() == 0) return bbTerm;
        if (bbTerm->getNumSuccessors() == 1) bbTerm = bbTerm->getSuccessor(0)->getTerminator();
        else return nullptr;
    }
    return nullptr;
}


} // namespace borealis
#include "Util/unmacros.h"

