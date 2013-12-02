/*
 * AdaptorPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

#include "Passes/Misc/AdaptorPass.h"
#include "Config/config.h"
#include "Codegen/intrinsics_manager.h"

namespace borealis {

namespace {

using namespace borealis::config;
MultiConfigEntry asserts{"adapt", "assert"};
MultiConfigEntry assumes{"adapt", "assume"};
MultiConfigEntry labels{"adapt", "error-label"};


llvm::Function* getBorealisBuiltin(
    function_type ft,
    llvm::Module& M
) {
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto argType = llvm::Type::getInt32Ty(M.getContext());
    auto retType = llvm::Type::getVoidTy(M.getContext());

    llvm::Function* cur = M.getFunction(
        intrinsic_manager.getFuncName(ft, "")
    );

    if(!cur) {
        // FIXME: name clashes still possible due to linking, research that
        auto* ty = llvm::FunctionType::get(
            retType,
            argType,
            false
        );

        cur = intrinsic_manager.createIntrinsic(
            ft,
            "",
            ty,
            &M
        );
    }

    return cur;
}

llvm::Value* mkBorealisBuiltin(
        function_type ft,
        llvm::Module& M,
        llvm::CallInst* existingCall
) {
    using llvm::dyn_cast;

    auto argType = llvm::Type::getInt32Ty(M.getContext());
    auto retType = llvm::Type::getVoidTy(M.getContext());

    llvm::Function* cur = getBorealisBuiltin(ft, M);

    auto* arg = existingCall->getArgOperand(0);
    llvm::Value* realArg = (arg->getType() == argType)? arg :
        llvm::CastInst::CreateIntegerCast(
            arg,
            llvm::Type::getInt32Ty(M.getContext()),
            false,
            "bor.builtin_arg",
            existingCall
        );
    auto* newCall = llvm::CallInst::Create(
        cur,
        realArg,
        "",
        existingCall
    );
    newCall->setDebugLoc(existingCall->getDebugLoc());
    newCall->setMetadata("dbg", existingCall->getMetadata("dbg"));

    if(existingCall->getType() != retType) {
        existingCall->replaceAllUsesWith(llvm::UndefValue::get(existingCall->getType()));
    } else {
        existingCall->replaceAllUsesWith(newCall);
    }
    existingCall->eraseFromParent();
    return newCall;
}

llvm::Value* mkBorealisAssert(
    llvm::Module& M,
    llvm::CallInst* existingCall) {
    return mkBorealisBuiltin(function_type::BUILTIN_BOR_ASSERT, M, existingCall);
}

llvm::Value* mkBorealisAssume(
    llvm::Module& M,
    llvm::CallInst* existingCall) {
    return mkBorealisBuiltin(function_type::BUILTIN_BOR_ASSUME, M, existingCall);
}

class CallVisitor : public llvm::InstVisitor<CallVisitor> {
public:

    // still buggy: unreachable propagation breaks everything to pieces
    void visitBasicBlock(llvm::BasicBlock& BB) {
        using namespace llvm;
        auto& M = *BB.getParent()->getParent();

        static auto label = util::viewContainer(labels.get()).toHashSet();
        if(BB.hasName() && label.count(BB.getName())) {
            auto first = BB.getFirstNonPHIOrDbgOrLifetime();
            auto f = getBorealisBuiltin(function_type::BUILTIN_BOR_ASSERT, M);
            auto arg = ConstantInt::get(f->getFunctionType()->getFunctionParamType(0), 0, false);

            llvm::CallInst::Create(getBorealisBuiltin(function_type::BUILTIN_BOR_ASSERT, M), arg, "", first);
        }
    }

    void visitCall(llvm::CallInst& I) {
        using namespace llvm;
        auto& M = *I.getParent()->getParent()->getParent();

        static auto assert = util::viewContainer(asserts.get()).toHashSet();
        static auto assume = util::viewContainer(assumes.get()).toHashSet();

        if(assert.count(I.getCalledFunction()->getName().str())) {
            mkBorealisAssert(M, &I);
        } else if(assume.count(I.getCalledFunction()->getName().str())) {
            mkBorealisAssume(M, &I);
        }
    }
};

} /* namespace */

////////////////////////////////////////////////////////////////////////////////

AdaptorPass::AdaptorPass() : ModulePass(ID) {}

void AdaptorPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
}

bool AdaptorPass::runOnModule(llvm::Module& M) {
    CallVisitor{}.visit(M);

    return false;
}

AdaptorPass::~AdaptorPass() {}

char AdaptorPass::ID;
static RegisterPass<AdaptorPass>
X("adaptor", "Adapt the borealis assertions api to custom functions or labels");

} /* namespace borealis */
