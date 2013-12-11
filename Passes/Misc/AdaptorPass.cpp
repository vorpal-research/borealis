/*
 * AdaptorPass.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: ice-phoenix
 */

#include <llvm/Constants.h>
#include <llvm/Support/InstVisitor.h>

#include "Codegen/intrinsics_manager.h"
#include "Config/config.h"
#include "Passes/Misc/AdaptorPass.h"

namespace borealis {

namespace {

using namespace borealis::config;
MultiConfigEntry asserts{"adapt", "assert"};
MultiConfigEntry assumes{"adapt", "assume"};
MultiConfigEntry labels {"adapt", "error-label"};
BoolConfigEntry undefinedDefaultsToUnknown{"adapt", "break-undefs"};

llvm::Function* getBorealisBuiltin(
    function_type ft,
    llvm::Module& M
) {
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto* argType = llvm::Type::getInt32Ty(M.getContext());
    auto* retType = llvm::Type::getVoidTy(M.getContext());

    llvm::Function* cur = M.getFunction(
        intrinsic_manager.getFuncName(ft, "")
    );

    if (!cur) {
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

    auto* argType = llvm::Type::getInt32Ty(M.getContext());
    auto* retType = llvm::Type::getVoidTy(M.getContext());

    llvm::Function* cur = getBorealisBuiltin(ft, M);

    auto* arg = existingCall->getArgOperand(0);
    llvm::Value* realArg = arg->getType() == argType ? arg :
        llvm::CastInst::CreateIntegerCast(
            arg,
            argType,
            false,
            "borealis.builtin_arg",
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

    if (existingCall->getType() != retType) {
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

llvm::Value* mkBorealisNonDet(llvm::Module* M, llvm::Type* type, llvm::Instruction* insertBefore) {
    auto& intrinsic_manager = IntrinsicsManager::getInstance();
    auto* func = intrinsic_manager.createIntrinsic(
        function_type::INTRINSIC_NONDET,
        util::toString(*type),
        llvm::FunctionType::get(type, false),
        M
    );

    return llvm::CallInst::Create(func, "", insertBefore);
}

class CallVisitor : public llvm::InstVisitor<CallVisitor> {
public:
    // XXX: still buggy, unreachable propagation breaks everything to pieces
    void visitBasicBlock(llvm::BasicBlock& BB) {
        using namespace llvm;
        auto& M = *BB.getParent()->getParent();

        static auto label = util::viewContainer(labels.get()).toHashSet();

        if (!BB.hasName()) return;

        // transform the ERROR labeled BB
        if (label.count(BB.getName())) {

            auto* first = BB.getFirstNonPHIOrDbgOrLifetime();
            auto* f = getBorealisBuiltin(function_type::BUILTIN_BOR_ASSERT, M);

            // Check if we've already been here and done stuff
            if (auto* CI = dyn_cast<CallInst>(first))
                if (f == CI->getCalledFunction())
                    return;

            auto* arg = ConstantInt::get(f->getFunctionType()->getFunctionParamType(0), 0, false);

            llvm::CallInst::Create(f, arg, "", first);
        }

        // define all undefined variables
        if (undefinedDefaultsToUnknown.get(false)) {
            std::unordered_set<const Value*> allocas;
            for (auto& I : BB) {
                if (auto* alloca = dyn_cast<AllocaInst>(&I)) {
                    allocas.insert(alloca);
                }

                if (auto* store = dyn_cast<StoreInst>(&I)) {
                    allocas.erase(store->getPointerOperand()->stripPointerCasts());
                }

                if (auto* load = dyn_cast<LoadInst>(&I)) {
                    auto* op = load->getPointerOperand();
                    auto it = allocas.find(op->stripPointerCasts());
                    if (it != allocas.end()) {
                        new StoreInst(
                            mkBorealisNonDet(&M, op->getType(), load),
                            op,
                            load
                        );
                        allocas.erase(it);
                    }
                }
            }
        }

    }

    void visitCall(llvm::CallInst& I) {
        using namespace llvm;

        static auto assert = util::viewContainer(asserts.get()).toHashSet();
        static auto assume = util::viewContainer(assumes.get()).toHashSet();

        auto* calledFunc = I.getCalledFunction();

        if (!calledFunc) return;
        if (!calledFunc->hasName()) return;

        auto& M = *I.getParent()->getParent()->getParent();
        if (assert.count(calledFunc->getName())) {
            mkBorealisAssert(M, &I);
        } else if (assume.count(calledFunc->getName())) {
            mkBorealisAssume(M, &I);
        }
    }

};

} /* namespace */

////////////////////////////////////////////////////////////////////////////////

AdaptorPass::AdaptorPass() : ModulePass(ID) {}

void AdaptorPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesCFG();
}

bool AdaptorPass::runOnModule(llvm::Module& M) {
    CallVisitor{}.visit(M);
    return false;
}

AdaptorPass::~AdaptorPass() {}

char AdaptorPass::ID;
static RegisterPass<AdaptorPass>
X("adaptor", "Adapt the borealis assertions API to custom functions or labels");

} /* namespace borealis */
