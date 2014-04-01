/*
 * llvm.cpp
 *
 *  Created on: Feb 25, 2013
 *      Author: ice-phoenix
 */

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/DIBuilder.h>
#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/IntrinsicInst.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>

#include "Codegen/FileManager.h"
#include "Codegen/llvm.h"

namespace borealis {

llvm::DebugLoc getFirstLocusForBlock(llvm::BasicBlock* bb) {
    for(auto& I : *bb) {
        const auto& dl = I.getDebugLoc();
        if(dl.isUnknown()) continue;

        return dl;
    }

    return llvm::DebugLoc{};
}

llvm::MDNode* getFirstMdNodeForBlock(llvm::BasicBlock* bb, const char* id) {
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
    MDNode* lmd;
    auto vec = util::viewContainer(*bb).map(util::takePtr{}).toVector();

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

void* MDNode2Ptr(llvm::MDNode* ptr) {
    if (ptr)
        if (auto* i = llvm::dyn_cast<llvm::ConstantInt>(ptr->getOperand(0)))
            return reinterpret_cast<void*>(static_cast<uintptr_t>(i->getLimitedValue()));
    return nullptr;
}

std::string getRawSource(const LocusRange& lr) {
    static FileManager fm;
    return fm.read(lr);
}

util::option<std::string> getAsCompileTimeString(llvm::Value* value) {
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

std::list<llvm::Constant*> getAsSeqData(llvm::Constant* value) {
    std::list<llvm::Constant*> res;

    auto* type = value->getType();

    if (type->isSingleValueType()) {
        res.push_back(value);

    } else if (auto* arrType = llvm::dyn_cast<llvm::ArrayType>(type)) {
        auto nested = util::range(0UL, arrType->getArrayNumElements())
            .map([&value](unsigned long i) { return value->getAggregateElement(i); })
            .map([](llvm::Constant* c) { return getAsSeqData(c); })
            .toList();
        res = util::viewContainer(nested).flatten().toList();

    } else if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        auto nested = util::range(0U, structType->getStructNumElements())
            .map([&value](unsigned i) { return value->getAggregateElement(i); })
            .map([](llvm::Constant* c) { return getAsSeqData(c); })
            .toList();
        res = util::viewContainer(nested).flatten().toList();

#include "Util/macros.h"
    } else BYE_BYE(decltype(res), "Unsupported constant-as-seq-data type: " + util::toString(type));
#include "Util/unmacros.h"

    return std::move(res);
}

std::set<DIType>& flattenTypeTree(DIType di, std::set<DIType>& collected) {
    if(collected.count(di)) return collected;
    collected.insert(di);

    if(DICompositeType struct_ = di) {
        auto members = struct_.getTypeArray();
        for(auto i = 0U; i < members.getNumElements(); ++i) {
            auto mem = members.getElement(i);
            flattenTypeTree(mem, collected);
        }
    } else if(DIDerivedType derived = di) {
        flattenTypeTree(derived.getTypeDerivedFrom(), collected);
    }

    return collected;
}

std::set<DIType> flattenTypeTree(DIType di) {
    std::set<DIType> collected;
    flattenTypeTree(di, collected);
    return std::move(collected);
}

DIType stripAliases(DIType tp) {
    if(DIAlias alias = tp) return stripAliases(alias.getOriginal());
    else return tp;
}

#include "Util/macros.h"
std::map<llvm::Type*, DIType>& flattenTypeTree(
    const std::pair<llvm::Type*, DIType>& tp,
    std::map<llvm::Type*, DIType>& collected) {

    auto* type = tp.first;
    auto di = stripAliases(tp.second);

    if(collected.count(type)) return collected;
    collected.insert({type, di});

    if(DICompositeType struct_ = di) {
        auto members = struct_.getTypeArray();
        ASSERTC(members.getNumElements() == type->getNumContainedTypes());
        for(auto i = 0U; i < members.getNumElements(); ++i) {
            auto mem = type->getContainedType(i);
            auto mmem = members.getElement(i);
            flattenTypeTree({mem, mmem}, collected);
        }
    } else if(DIDerivedType derived = di) {
        flattenTypeTree({type->getContainedType(0), derived.getTypeDerivedFrom()}, collected);
    }

    return collected;
}
#include "Util/unmacros.h"

std::map<llvm::Type*, DIType> flattenTypeTree(const std::pair<llvm::Type*, DIType>& tp) {
    std::map<llvm::Type*, DIType> collected;
    flattenTypeTree(tp, collected);
    return std::move(collected);
}

//===----------------------------------------------------------------------===//
// DebugInfoFinder implementations.
//===----------------------------------------------------------------------===//

/// processModule - Process entire module and collect debug info.
void DebugInfoFinder::processModule(llvm::Module &M) {
    if (auto CU_Nodes = M.getNamedMetadata("llvm.dbg.cu")) {
        for (unsigned i = 0, e = CU_Nodes->getNumOperands(); i != e; ++i) {
            llvm::DICompileUnit CU(CU_Nodes->getOperand(i));
            addCompileUnit(CU);
            if (CU.getVersion() > llvm::LLVMDebugVersion10) {
                llvm::DIArray GVs = CU.getGlobalVariables();
                for (unsigned i = 0, e = GVs.getNumElements(); i != e; ++i) {
                    llvm::DIGlobalVariable DIG(GVs.getElement(i));
                    if (addGlobalVariable(DIG))
                        processType(DIG.getType());
                }
                llvm::DIArray SPs = CU.getSubprograms();
                for (unsigned i = 0, e = SPs.getNumElements(); i != e; ++i)
                    processSubprogram(llvm::DISubprogram(SPs.getElement(i)));
                llvm::DIArray EnumTypes = CU.getEnumTypes();
                for (unsigned i = 0, e = EnumTypes.getNumElements(); i != e; ++i)
                    processType(DIType(EnumTypes.getElement(i)));
                llvm::DIArray RetainedTypes = CU.getRetainedTypes();
                for (unsigned i = 0, e = RetainedTypes.getNumElements(); i != e; ++i)
                    processType(DIType(RetainedTypes.getElement(i)));

            }
        }
    }

    for (auto I = M.begin(), E = M.end(); I != E; ++I)
        for (auto FI = (*I).begin(), FE = (*I).end(); FI != FE; ++FI)
            for (auto BI = (*FI).begin(), BE = (*FI).end(); BI != BE;
                ++BI) {
                if (llvm::DbgDeclareInst *DDI = llvm::dyn_cast<llvm::DbgDeclareInst>(BI))
                    processDeclare(DDI);

                llvm::DebugLoc Loc = BI->getDebugLoc();
                if (Loc.isUnknown())
                    continue;

                llvm::LLVMContext &Ctx = BI->getContext();
                llvm::DIDescriptor Scope(Loc.getScope(Ctx));

                if (Scope.isCompileUnit())
                    addCompileUnit(llvm::DICompileUnit(Scope));
                else if (Scope.isSubprogram())
                    processSubprogram(llvm::DISubprogram(Scope));
                else if (Scope.isLexicalBlockFile()) {
                    llvm::DILexicalBlockFile DBF = llvm::DILexicalBlockFile(Scope);
                    processLexicalBlock(llvm::DILexicalBlock(DBF.getScope()));
                }
                else if (Scope.isLexicalBlock())
                    processLexicalBlock(llvm::DILexicalBlock(Scope));

                if (llvm::MDNode *IA = Loc.getInlinedAt(Ctx))
                    processLocation(llvm::DILocation(IA));
            }

    if (auto NMD = M.getNamedMetadata("llvm.dbg.gv")) {
        for (unsigned i = 0, e = NMD->getNumOperands(); i != e; ++i) {
            llvm::DIGlobalVariable DIG(llvm::cast<llvm::MDNode>(NMD->getOperand(i)));
            if (addGlobalVariable(DIG)) {
                if (DIG.getVersion() <= llvm::LLVMDebugVersion10)
                    addCompileUnit(DIG.getCompileUnit());
                processType(DIG.getType());
            }
        }
    }

    if (auto NMD = M.getNamedMetadata("llvm.dbg.sp"))
        for (unsigned i = 0, e = NMD->getNumOperands(); i != e; ++i)
            processSubprogram(llvm::DISubprogram(NMD->getOperand(i)));
}

/// processLocation - Process DILocation.
void DebugInfoFinder::processLocation(llvm::DILocation Loc) {
    if (!Loc.Verify()) return;
    llvm::DIDescriptor S(Loc.getScope());
    if (S.isCompileUnit())
        addCompileUnit(llvm::DICompileUnit(S));
    else if (S.isSubprogram())
        processSubprogram(llvm::DISubprogram(S));
    else if (S.isLexicalBlock())
        processLexicalBlock(llvm::DILexicalBlock(S));
    else if (S.isLexicalBlockFile()) {
        llvm::DILexicalBlockFile DBF = llvm::DILexicalBlockFile(S);
        processLexicalBlock(llvm::DILexicalBlock(DBF.getScope()));
    }
    processLocation(Loc.getOrigLocation());
}

/// processType - Process DIType.
void DebugInfoFinder::processType(llvm::DIType DT) {
    if (!addType(DT))
        return;
    if (DT.getVersion() <= llvm::LLVMDebugVersion10)
        addCompileUnit(DT.getCompileUnit());
    if (DT.isCompositeType()) {
        llvm::DICompositeType DCT(DT);
        processType(DCT.getTypeDerivedFrom());
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
        processType(DDT.getTypeDerivedFrom());
    }
}

/// processLexicalBlock
void DebugInfoFinder::processLexicalBlock(llvm::DILexicalBlock LB) {
    llvm::DIScope Context = LB.getContext();
    if (Context.isLexicalBlock())
        return processLexicalBlock(llvm::DILexicalBlock(Context));
    else if (Context.isLexicalBlockFile()) {
        llvm::DILexicalBlockFile DBF = llvm::DILexicalBlockFile(Context);
        return processLexicalBlock(llvm::DILexicalBlock(DBF.getScope()));
    }
    else
        return processSubprogram(llvm::DISubprogram(Context));
}

/// processSubprogram - Process DISubprogram.
void DebugInfoFinder::processSubprogram(llvm::DISubprogram SP) {
    if (!addSubprogram(SP))
        return;
    if (SP.getVersion() <= llvm::LLVMDebugVersion10)
        addCompileUnit(SP.getCompileUnit());
    processType(SP.getType());
}

/// processDeclare - Process DbgDeclareInst.
void DebugInfoFinder::processDeclare(llvm::DbgDeclareInst *DDI) {
    llvm::MDNode *N = llvm::dyn_cast<llvm::MDNode>(DDI->getVariable());
    if (!N) return;

    llvm::DIDescriptor DV(N);
    if (!DV.isVariable())
        return;

    if (!NodesSeen.insert(DV))
        return;
    if (llvm::DIVariable(N).getVersion() <= llvm::LLVMDebugVersion10)
        addCompileUnit(llvm::DIVariable(N).getCompileUnit());
    processType(llvm::DIVariable(N).getType());
}

/// addType - Add type into Tys.
bool DebugInfoFinder::addType(llvm::DIType DT) {
    if (!DT.isValid())
        return false;

    if (!NodesSeen.insert(DT))
        return false;

    TYs.push_back(DT);
    return true;
}

/// addCompileUnit - Add compile unit into CUs.
bool DebugInfoFinder::addCompileUnit(llvm::DICompileUnit CU) {
    if (!CU.Verify())
        return false;

    if (!NodesSeen.insert(CU))
        return false;

    CUs.push_back(CU);
    return true;
}

/// addGlobalVariable - Add global variable into GVs.
bool DebugInfoFinder::addGlobalVariable(llvm::DIGlobalVariable DIG) {
    if (!llvm::DIDescriptor(DIG).isGlobalVariable())
        return false;

    if (!NodesSeen.insert(DIG))
        return false;

    GVs.push_back(DIG);
    return true;
}

// addSubprogram - Add subprgoram into SPs.
bool DebugInfoFinder::addSubprogram(llvm::DISubprogram SP) {
    if (!llvm::DIDescriptor(SP).isSubprogram())
        return false;

    if (!NodesSeen.insert(SP))
        return false;

    SPs.push_back(SP);
    return true;
}

} // namespace borealis
