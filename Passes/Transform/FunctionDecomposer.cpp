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
#include <llvm/Transforms/Utils/PromoteMemToReg.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

#include "Annotation/AnnotationCast.h"
#include "Codegen/intrinsics_manager.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Transform/AnnotationProcessor.h"
#include "Passes/Transform/FunctionDecomposer.h"
#include "Passes/Transform/MetaInserter.h"
#include "State/Transformer/ExternalFunctionMaterializer.h"
#include "State/Transformer/AnnotationMaterializer.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "State/Transformer/OldValueExtractor.h"
#include "Statistics/statistics.h"
#include "Util/passes.hpp"
#include "Passes/Misc/FuncInfoProvider.h"
#include "Passes/Transform/MallocMutator.h"

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

    auto&& name = "bor.dc.load." + originalCall.getCalledFunction()->getName()
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

    auto&& name = "bor.dc.nondet." + originalCall.getCalledFunction()->getName()
                + (originalCall.hasName() ? "." + originalCall.getName() : "")
                + ".arg" + llvm::Twine(argNum);

    auto&& call = createCall(f, name, &originalCall);

    return new llvm::StoreInst(call, arg, &originalCall);
}

inline llvm::CallInst* mkNondet(
        IntrinsicsManager& IM,
        llvm::Module& M,
        llvm::Type* type,
        const std::string& modifier,
        llvm::Instruction* insertBefore) {
    auto&& f = IM.createIntrinsic(
        function_type::INTRINSIC_NONDET,
        util::toString(*type),
        llvm::FunctionType::get(type, false),
        &M
    );

    f->setDoesNotAccessMemory();
    f->setDoesNotThrow();

    auto&& name = "bor.dc.nondet." + modifier;

    return createCall(f, name, insertBefore);
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

    auto&& name = "bor.dc.nondet." + originalCall.getCalledFunction()->getName()
                + (originalCall.hasName() ? "." + originalCall.getName() : "");

    return createCall(f, name, &originalCall);
}

static std::vector<std::string> implicitContracts(const llvm::Function* func, const func_info::FuncInfo& fi) {
    std::vector<std::string> ret;

    auto&& ri = fi.resultInfo;

    if(func->getReturnType()->isPointerTy()) {
        if(ri.isArray == func_info::ArrayTag::IsArray && ri.sizeArgument) {
            auto&& sz = ri.sizeArgument.getUnsafe();
            ret.push_back(tfm::format("@ensures \\is_valid_array(\\result, \\arg%d) ", sz));
        }
    }

    for(size_t i = 0; i < fi.argInfo.size(); ++i) {
        auto ptype = func->isVarArg()? func->getFunctionType()->getParamType(i) : nullptr;
        if(!ptype || ptype->isPointerTy()) {
            auto&& ai = fi.argInfo[i];
            if(ai.access != func_info::AccessPatternTag::None) {
                if(ai.isArray == func_info::ArrayTag::IsArray && ai.sizeArgument) {
                    ret.push_back(tfm::format("@requires[[BUF-01]] \\is_valid_array(\\arg%d, \\arg%d) ", i, ai.sizeArgument.getUnsafe()));
                } else {
                    ret.push_back(tfm::format("@requires[[BUF-01]] \\is_valid_ptr(\\arg%d) ", i));
                }
            }
        }
    }

    return std::move(ret);
}

void FunctionDecomposer::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
    AUX<llvm::TargetLibraryInfo>::addRequiredTransitive(AU);
    AUX<borealis::FuncInfoProvider>::addRequiredTransitive(AU);
    AUX<borealis::SlotTrackerPass>::addRequiredTransitive(AU);
    AUX<borealis::VariableInfoTracker>::addRequiredTransitive(AU);
    AUX<borealis::SourceLocationTracker>::addRequiredTransitive(AU);

    AUX<llvm::DominatorTreeWrapperPass>::addRequired(AU);
}


static void manualMem2Reg(llvm::Function* f, llvm::DominatorTree& DT) {

    while(1) {
        auto allocas =
            util::viewContainer(*f)
                .flatten()
                .map(ops::take_pointer)
                .map(llvm::ops::dyn_cast<llvm::AllocaInst>)
                .filter()
                .filter(llvm::isAllocaPromotable)
                .toVector();
        if(allocas.empty()) break;

        llvm::PromoteMemToReg(allocas, DT);
    }

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
            // && !llvm::isAllocationFn(ci, &TLI)
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
                 .map(llvm::ops::dyn_cast<llvm::CallInst>)
                 .filter(isDecomposable)
                 .toHashSet();

    //std::unordered_map<llvm::CallInst*, llvm::Value*> replacements;
    llvm::DenseMap<llvm::CallInst*, llvm::WeakVH> replacements;

    for(llvm::CallInst* call : funcs) {
        llvm::Function* f = call->getCalledFunction();
        llvm::Value* predefinedReturn = nullptr;
        auto loc = SLT.getLocFor(call);
        if (FIP.hasInfo(f)) {

            auto&& funcInfo = FIP.getInfo(f);

            infos() << "FIPping " << f->getName() << endl;
            FactoryNest FN(M.getDataLayout(), STP.getSlotTracker(call));

            auto contracts = FIP.getContracts(f);
            auto impContracts = util::viewContainer(implicitContracts(f, funcInfo))
                                .map(LAM(A, fromString(Locus{}, A, FN.Term)))
                                .filter()
                                .toVector();

            contracts.insert(std::end(contracts), std::begin(impContracts), std::end(impContracts));

            std::vector<Annotation::Ptr> beforeAlls;
            std::vector<Annotation::Ptr> befores;
            std::vector<Annotation::Ptr> afters;
            std::vector<Annotation::Ptr> middles;

            for (auto contract : contracts) {
                try{
                    OldValueExtractor ove(FN);
                    contract = ove.transform(contract);
                    befores.insert(befores.end(), ove.getResults().begin(), ove.getResults().end());
                    if(auto&& la = dyn_cast<LogicAnnotation>(contract)) {
                        AnnotationMaterializer AM(*la, FN, &VIT, call->getCalledFunction());
                        contract = AM.transform(contract);
                    }

                    CallSiteInitializer CSI(call, FN, &loc);
                    auto onCallContract = CSI.transform(contract);
                    if(llvm::isa<GlobalAnnotation>(contract)) {
                        beforeAlls.push_back(onCallContract);
                    } else if (llvm::is_one_of<EnsuresAnnotation, AssumeAnnotation>(contract)) {
                        afters.push_back(onCallContract);
                    } else if (llvm::is_one_of<RequiresAnnotation, AssertAnnotation>(contract)) {
                        befores.push_back(onCallContract);
                    } else {
                        middles.push_back(onCallContract);
                    }
                } catch(std::exception& ex) {
                    // FIXME: this is generally fucked up
                    auto log = llvm::isa<GlobalAnnotation>(contract)? infos() : errs();
                    log << "Unable to materialize annotation " << *contract << ": " << ex.what() << endl;
                }
            }

            for (auto&& before: befores) AnnotationProcessor::landOnInstructionOrFirst(before, M, FN, *call);
            for (auto&& middle: middles) AnnotationProcessor::landOnInstructionOrFirst(middle, M, FN, *call);
            for (auto&& beforeAll: beforeAlls) {
                AnnotationProcessor::landOnInstructionOrFirst(beforeAll, M, FN, *call->getParent()->getParent());
            }

            for (auto i = 0U; i < call->getNumArgOperands(); ++i) {
                auto realIx = f->isVarArg() && i > f->getArgumentList().size() ? f->getArgumentList().size() : i;
                auto&& argInfo = funcInfo.argInfo.at(realIx);

                auto&& arg = call->getArgOperand(i);

                if (FIP.getInfo(f).resultInfo.boundArgument == size_t(i)) {
                    predefinedReturn = arg;
                }

                auto writeOnly = argInfo.access == func_info::AccessPatternTag::Write;
                auto readOnly = argInfo.access == func_info::AccessPatternTag::Read;
                auto noAccess = argInfo.access == func_info::AccessPatternTag::None;

                if (!arg->getType()->isPointerTy()
                    || arg->getType()->getPointerElementType()->isFunctionTy()
                    || arg->getType()->getPointerElementType()->isAggregateType()
                    || llvm::isa<llvm::Constant>(arg)
                    || noAccess
                    || argInfo.isArray == func_info::ArrayTag::IsArray) {
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

            if(funcInfo.resultInfo.special == func_info::SpecialTag::Malloc) {
                llvm::Value* sizeArgument = nullptr;
                if(funcInfo.resultInfo.sizeArgument) {
                    sizeArgument = call->getArgOperand(funcInfo.resultInfo.sizeArgument.getUnsafe());
                } else {
                    sizeArgument = mkNondet(
                                        IM,
                                        M,
                                        llvm::Type::getInt64Ty(M.getContext()),
                                        "allocIndex",
                                        call
                                   );
                }

                MallocMutator::mutateMemoryInst(
                    M,
                    call,
                    function_type::INTRINSIC_MALLOC,
                    call->getType(),
                    call->getType()->getPointerElementType(),
                    sizeArgument,
                    [call, &predefinedReturn](llvm::Instruction*, llvm::Instruction* new_) {
                        new_->setName("bor.dc.malloc." + call->getCalledFunction()->getName());
                        predefinedReturn = new_;
                    }
                );

                if(call->getType() != predefinedReturn->getType()) {
                    auto cast = llvm::CastInst::CreatePointerCast(predefinedReturn, call->getType(), "bor.dc.malloc.backcast", call);
                    predefinedReturn = cast;
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

    // FIXME: this is insanely fucked up
    MetaInserter::unliftAllDebugIntrinsics(M);
    for(auto&& F: M) if(!F.isDeclaration()) {
        auto&& DT = GetAnalysis<llvm::DominatorTreeWrapperPass>::doit(this, F).getDomTree();
        manualMem2Reg(&F, DT);
    }
    MetaInserter::liftAllDebugIntrinsics(M);

    return false;
}


void FunctionDecomposer::print(llvm::raw_ostream&, const llvm::Module*) const {

}

}; /* namespace borealis */

#include "Util/unmacros.h"
