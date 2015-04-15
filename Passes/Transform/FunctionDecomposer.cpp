/*
 * FunctionDecomposer.cpp
 *
 *  Created on: Jan 23, 2015
 *      Author: belyaev
 */



#include <string>
#include <unordered_set>

#include <llvm/Analysis/MemoryBuiltins.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetLibraryInfo.h>

#include "Passes/Transform/FunctionDecomposer.h"
#include "Codegen/intrinsics_manager.h"
#include "Statistics/statistics.h"
#include "Util/passes.hpp"
#include "Config/config.h"
#include "Util/functional.hpp"
#include "Passes/Misc/FuncInfoProvider.h"

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

#include "Util/macros.h"

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
    f->setAttributes(originalCall.getCalledFunction()->getAttributes());
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
    f->setAttributes(originalCall.getCalledFunction()->getAttributes());
    f->setDoesNotAccessMemory();
    f->setDoesNotThrow();

    auto&& name = "bor.decomposed.nondet." + originalCall.getCalledFunction()->getName()
                + (originalCall.hasName() ? "." + originalCall.getName() : "")
                + ".res";

    return createCall(f, name, &originalCall);
}
#include "Util/unmacros.h"

void FunctionDecomposer::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
    AUX<llvm::TargetLibraryInfo>::addRequiredTransitive(AU);
    AUX<borealis::FuncInfoProvider>::addRequiredTransitive(AU);
}

bool FunctionDecomposer::runOnModule(llvm::Module& M) {
    using namespace borealis::util;

    auto excludedFunctionNames = viewContainer(excludeFunctions).toHashSet();

    auto&& TLI = GetAnalysis<llvm::TargetLibraryInfo>::doit(this);

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

    for(llvm::CallInst* call : funcs) {
        llvm::Function* f = call->getCalledFunction();
        llvm::Value* predefinedReturn = nullptr;

        for(auto i = 0U; i < call->getNumArgOperands(); ++i) {
            auto&& arg = call->getArgOperand(i);
            if(call->getCalledFunction()->getAttributes().hasAttribute(i, llvm::Attribute::Returned)) {
                predefinedReturn = arg;
            }

            if(!arg->getType()->isPointerTy()
             || arg->getType()->getPointerElementType()->isFunctionTy()
             || llvm::isa<llvm::Constant>(arg)
             || call->getCalledFunction()->doesNotAccessMemory(i)) {
                auto&& consume = mkConsumeCall(IM, M, *call, i);
                if(auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
            } else if(call->getCalledFunction()->onlyReadsMemory(i)) {
                auto&& load = mkLoad(*call, i);
                if(auto&& md = call->getMetadata("dbg")) load->setMetadata("dbg", md);
                auto&& consume = mkConsumeCall(IM, M, *call, i, load);
                if(auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
            } else {
                auto&& load = mkLoad(*call, i);
                if(auto&& md = call->getMetadata("dbg")) load->setMetadata("dbg", md);
                auto&& consume = mkConsumeCall(IM, M, *call, i, load);
                if(auto&& md = call->getMetadata("dbg")) consume->setMetadata("dbg", md);
                auto&& store = mkStoreNondet(IM, M, *call, i);
                if(auto&& md = call->getMetadata("dbg")) store->setMetadata("dbg", md);
            }
        }
        if(predefinedReturn) {
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
