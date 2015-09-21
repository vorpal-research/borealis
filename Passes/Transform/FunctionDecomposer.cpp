/*
 * FunctionDecomposer.cpp
 *
 *  Created on: Jan 23, 2015
 *      Author: belyaev
 */



#include <string>
#include <unordered_set>

#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Codegen/intrinsics_manager.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Transform/AnnotationProcessor.h"
#include "Passes/Transform/FunctionDecomposer.h"
#include "State/Transformer/ExternalFunctionMaterializer.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "State/Transformer/OldValueExtractor.h"
#include "Statistics/statistics.h"
#include "Util/passes.hpp"
#include "Passes/Misc/FuncInfoProvider.h"

#include "Util/macros.h"

namespace borealis {

namespace lfn = llvm::LibFunc;

char FunctionDecomposer::ID;
static RegisterPass<FunctionDecomposer>
X("decompose-functions", "Replace all unknown functions with corresponding borealis intrinsics");

static Statistic FunctionsDecomposed("decompose-functions",
    "totalFunctions", "Total number of function calls decomposed");

static config::MultiConfigEntry excludeFunctions{ "decompose-functions", "exclude" };

struct FunctionDecomposer::Impl {};

FunctionDecomposer::FunctionDecomposer() : llvm::ModulePass{ID}, pimpl_{ new Impl{} } {}

FunctionDecomposer::~FunctionDecomposer() {}

inline llvm::CallInst* createCall(llvm::Function* what, llvm::Value* arg,
    llvm::Twine name, llvm::Instruction* insertBefore) {
    bool noName = what->getReturnType()->isVoidTy();
    return llvm::CallInst::Create(what, arg, noName? "" : name, insertBefore);
}

inline llvm::CallInst* createCall(llvm::Function* what, llvm::Twine name, llvm::Instruction* insertBefore) {
    bool noName = what->getReturnType()->isVoidTy();
    return llvm::CallInst::Create(what, noName? "" : name, insertBefore);
}

inline llvm::CallInst* mkConsumeCall(
        IntrinsicsManager& IM,
        llvm::Module& M,
        llvm::CallInst& originalCall,
        size_t argNum,
        llvm::Value* arg = nullptr
    ) {
    if(!arg) arg = originalCall.getArgOperand(argNum);

    auto&& f = IM.createIntrinsic(
        function_type::INTRINSIC_CONSUME,
        util::toString(*arg->getType()),
        llvm::FunctionType::get(llvm::Type::getVoidTy(M.getContext()), arg->getType(), false),
        &M
    );

//    auto&& name = "bor.decomposed." + originalCall.getCalledFunction()->getName()
//                + (originalCall.hasName() ? "." + originalCall.getName() : "")
//                + ".arg" + llvm::Twine(argNum);

    return createCall(f, arg, "", &originalCall);
}

inline llvm::Instruction* mkLoad(
        llvm::CallInst& originalCall,
        size_t argNum,
        llvm::Value* arg = nullptr) {
    if(!arg) arg = originalCall.getArgOperand(argNum);

    ASSERTC(arg->getType()->isPointerTy());

    auto&& name = "bor.decomposed.load." + originalCall.getCalledFunction()->getName()
                + (originalCall.hasName() ? "." + originalCall.getName() : "")
                + ".arg" + llvm::Twine(argNum);

    return new llvm::LoadInst(arg, name, &originalCall);
}

inline llvm::Instruction* mkStoreNondet(
        IntrinsicsManager& IM,
        llvm::Module& M,
        llvm::CallInst& originalCall,
        size_t argNum,
        llvm::Value* arg = nullptr) {
    if(!arg) arg = originalCall.getArgOperand(argNum);

    ASSERTC(arg->getType()->isPointerTy());

    auto&& f = IM.createIntrinsic(
        function_type::INTRINSIC_NONDET,
        util::toString(*arg->getType()->getPointerElementType()),
        llvm::FunctionType::get(arg->getType()->getPointerElementType(), false),
        &M
    );

    auto&& attrSet = originalCall.getCalledFunction()->getAttributes();
    attrSet = attrSet.addAttributes(M.getContext(), llvm::AttributeSet::FunctionIndex,
                                    originalCall.getCalledFunction()->getAttributes().getFnAttributes());
    attrSet = attrSet.removeAttribute(M.getContext(), llvm::AttributeSet::FunctionIndex, llvm::Attribute::AttrKind::ReadNone);
    attrSet = attrSet.removeAttribute(M.getContext(), llvm::AttributeSet::FunctionIndex, llvm::Attribute::AttrKind::ReadOnly);

    f->setAttributes(attrSet);
    f->setDoesNotAccessMemory();
    f->setDoesNotThrow();

    auto&& name = "bor.decomposed.nondet." + originalCall.getCalledFunction()->getName()
                + (originalCall.hasName() ? "." + originalCall.getName() : "")
                + ".arg" + llvm::Twine(argNum);

    auto&& call = createCall(f, name, &originalCall);

    return new llvm::StoreInst(call, arg, &originalCall);
}

inline llvm::CallInst* mkNondet(
        IntrinsicsManager& IM,
        llvm::Module& M,
        llvm::CallInst& originalCall) {
    auto&& f = IM.createIntrinsic(
        function_type::INTRINSIC_NONDET,
        util::toString(*originalCall.getType()),
        llvm::FunctionType::get(originalCall.getType(), false),
        &M
    );

    auto&& attrSet = llvm::AttributeSet();
    attrSet = attrSet.addAttributes(M.getContext(), llvm::AttributeSet::FunctionIndex,
                                    originalCall.getCalledFunction()->getAttributes().getFnAttributes());
    attrSet = attrSet.removeAttribute(M.getContext(), llvm::AttributeSet::FunctionIndex, llvm::Attribute::AttrKind::ReadNone);
    attrSet = attrSet.removeAttribute(M.getContext(), llvm::AttributeSet::FunctionIndex, llvm::Attribute::AttrKind::ReadOnly);

    f->setAttributes(attrSet);
    f->setDoesNotAccessMemory();
    f->setDoesNotThrow();

    auto&& name = "bor.decomposed.nondet." + originalCall.getCalledFunction()->getName()
                + (originalCall.hasName() ? "." + originalCall.getName() : "")
                + ".res";

    return createCall(f, name, &originalCall);
}

void FunctionDecomposer::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
    AUX<llvm::TargetLibraryInfo>::addRequiredTransitive(AU);
    AUX<borealis::FuncInfoProvider>::addRequiredTransitive(AU);
    AUX<borealis::SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<borealis::VariableInfoTracker>::addRequiredTransitive(AU);
    AUX<borealis::SourceLocationTracker>::addRequiredTransitive(AU);
}

bool FunctionDecomposer::runOnModule(llvm::Module& M) {
    using namespace borealis::util;

    auto excludedFunctionNames = viewContainer(excludeFunctions).toHashSet();

    auto&& TLI = GetAnalysis<llvm::TargetLibraryInfo>::doit(this);
    auto&& STP = GetAnalysis<borealis::SlotTrackerPass>::doit(this);
    auto&& VIT = GetAnalysis<borealis::VariableInfoTracker>::doit(this);
    auto&& SLT = GetAnalysis<borealis::SourceLocationTracker>::doit(this);

    auto&& IM = IntrinsicsManager::getInstance();
    auto&& FIP = GetAnalysis<FuncInfoProvider>::doit(this);

    auto isDecomposable = [&](const llvm::CallInst* ci) {
        if(!ci) return false;

        auto&& func = ci->getCalledFunction();
        return func
            && func->isDeclaration()
            && !func->isIntrinsic()
            && !llvm::isAllocationFn(ci, &TLI)
            && !llvm::isFreeCall(ci, &TLI)
            && !func->doesNotReturn()
            // FIXME
            && !(func->getName().startswith("borealis_"))
            && IM.getIntrinsicType(func) == function_type::UNKNOWN
            && !excludedFunctionNames.count(func->getName().str());
    };

    auto funcs =  viewContainer(M)
                 .flatten()
                 .flatten()
                 .map(ops::take_pointer)
                 .map(llvm::dyn_caster<llvm::CallInst>{})
                 .filter(isDecomposable)
                 .toHashSet();

    std::unordered_map<llvm::CallInst*, llvm::Value*> replacements;

    for(llvm::CallInst* call : funcs) {
        llvm::Function* f = call->getCalledFunction();
        llvm::Value* predefinedReturn = nullptr;
        if (FIP.hasInfo(f)) {
            infos() << "FIPping " << f->getName() << endl;
            FactoryNest FN(STP.getSlotTracker(call));

            auto&& contracts = FIP.getContracts(f);
            std::vector<Annotation::Ptr> befores;
            std::vector<Annotation::Ptr> afters;
            std::vector<Annotation::Ptr> middles;

            for (auto contract : contracts) {
                ExternalFunctionMaterializer efm(FN, llvm::CallSite(call), &VIT, SLT.getLocFor(call));
                contract = efm.transform(contract);
                OldValueExtractor ove(FN);
                contract = ove.transform(contract);
                befores.insert(befores.end(), ove.getResults().begin(), ove.getResults().end());
                contract = materialize(contract, FN, &VIT);
                if (llvm::is_one_of<EnsuresAnnotation, AssumeAnnotation>(contract)) {
                    afters.push_back(contract);
                } else if (llvm::is_one_of<RequiresAnnotation, AssertAnnotation>(contract)) {
                    befores.push_back(contract);
                } else {
                    middles.push_back(contract);
                }
            }

            for (auto&& before: befores) AnnotationProcessor::landOnInstructionOrFirst(before, M, FN, *call);
            for (auto&& middle: middles) AnnotationProcessor::landOnInstructionOrFirst(middle, M, FN, *call);

            for (auto i = 0U; i < call->getNumArgOperands(); ++i) {
                auto realIx = f->isVarArg() && i > f->getArgumentList().size() ? f->getArgumentList().size() : i;

                auto&& arg = call->getArgOperand(i);

                auto writeOnly = FIP.getInfo(f).argInfo.at(realIx).access == func_info::AccessPatternTag::Write;
                auto readOnly = FIP.getInfo(f).argInfo.at(realIx).access == func_info::AccessPatternTag::Read;
                auto noAccess = FIP.getInfo(f).argInfo.at(realIx).access == func_info::AccessPatternTag::None;
                if (FIP.getInfo(f).resultInfo.sizeArgument == size_t(i)) {
                    predefinedReturn = arg;
                }

                if (!arg->getType()->isPointerTy()
                    || arg->getType()->getPointerElementType()->isFunctionTy()
                    || llvm::isa<llvm::Constant>(arg)
                    || noAccess) {
                    auto&& consume = mkConsumeCall(IM, M, *call, i);
                    if (auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                } else if (readOnly) {
                    auto&& load = mkLoad(*call, i);
                    if (auto&& md = call->getMetadata("dbg")) load->setMetadata("dbg", md);
                    auto&& consume = mkConsumeCall(IM, M, *call, i, load);
                    if (auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                } else if (writeOnly) {
                    auto&& store = mkStoreNondet(IM, M, *call, i);
                    if (auto&& md = call->getMetadata("dbg")) store->setMetadata("dbg", md);
                } else {
                    auto&& load = mkLoad(*call, i);
                    if (auto&& md = call->getMetadata("dbg")) load->setMetadata("dbg", md);
                    auto&& consume = mkConsumeCall(IM, M, *call, i, load);
                    if (auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                    auto&& store = mkStoreNondet(IM, M, *call, i);
                    if (auto&& md = call->getMetadata("dbg")) store->setMetadata("dbg", md);
                }
            }

            for (auto&& require: afters) AnnotationProcessor::landOnInstructionOrLast(require, M, FN, *call);
        } else {
            for (auto i = 0U; i < call->getNumArgOperands(); ++i) {
                auto&& arg = call->getArgOperand(i);
                if (call->getCalledFunction()->getAttributes().hasAttribute(i, llvm::Attribute::Returned)) {
                    predefinedReturn = arg;
                }

                if (!arg->getType()->isPointerTy()
                    || arg->getType()->getPointerElementType()->isAggregateType() // FIXME; think better
                    || arg->getType()->getPointerElementType()->isFunctionTy()
                    || llvm::isa<llvm::Constant>(arg)
                    || call->getCalledFunction()->doesNotAccessMemory(i)) {
                    auto&& consume = mkConsumeCall(IM, M, *call, i);
                    if (auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                } else if (call->getCalledFunction()->onlyReadsMemory(i)) {
                    auto&& load = mkLoad(*call, i);
                    if (auto&& md = call->getMetadata("dbg")) load->setMetadata("dbg", md);
                    auto&& consume = mkConsumeCall(IM, M, *call, i, load);
                    if (auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                } else {
                    auto&& load = mkLoad(*call, i);
                    if (auto&& md = call->getMetadata("dbg")) load->setMetadata("dbg", md);
                    auto&& consume = mkConsumeCall(IM, M, *call, i, load);
                    if (auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                    auto&& store = mkStoreNondet(IM, M, *call, i);
                    if (auto&& md = call->getMetadata("dbg")) store->setMetadata("dbg", md);
                }
            }
        }

        replacements[call] = predefinedReturn;
    }
    for(auto&& call : funcs){
        auto predefinedReturn = replacements[call];
        if (predefinedReturn) {
            call->replaceAllUsesWith(predefinedReturn);
        } else {
            auto&& replacementCall = mkNondet(IM, M, *call);
            call->replaceAllUsesWith(replacementCall);
        }
        call->eraseFromParent();

        FunctionsDecomposed++;
    }

    return false;
}

void FunctionDecomposer::print(llvm::raw_ostream&, const llvm::Module*) const {

}

}; /* namespace borealis */

#include "Util/unmacros.h"
