/*
 * MetaInserter.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#include <llvm/Support/Casting.h>
#include <llvm/Support/InstIterator.h>

#include "Passes/MetaInserter.h"

namespace borealis {

MetaInserter::MetaInserter() : ModulePass(ID) {}

MetaInserter::~MetaInserter() {}

bool MetaInserter::runOnModule(llvm::Module &M) {
    using borealis::util::toString;
    using borealis::util::view;

    using llvm::inst_begin;
    using llvm::inst_end;
    using llvm::dyn_cast;
    using llvm::BasicBlock;
    using llvm::CallInst;
    using llvm::LoadInst;
    using llvm::ReturnInst;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    llvm::DebugInfoFinder dfi;
    dfi.processModule(M);

    llvm::Function* Desc = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
            "",
            llvm::FunctionType::get(llvm::Type::getVoidTy(M.getContext()), false),
            &M
    );
    BasicBlock::Create(M.getContext(), "bb", Desc);

    for (auto& mglob : view(dfi.global_variable_begin(), dfi.global_variable_end())) {
        llvm::DIDescriptor di(mglob);
        if (!di.isGlobalVariable()) continue;

        llvm::DIGlobalVariable glob(mglob);
        if (!glob.getGlobal()) continue;

        auto* current = intrinsic_manager.createIntrinsic(
                function_type::INTRINSIC_GLOBAL,
                toString(*glob.getGlobal()->getType()->getPointerElementType()),
                llvm::FunctionType::get(
                        llvm::Type::getVoidTy(M.getContext()),
                        std::vector<llvm::Type*> { glob.getGlobal()->getType()->getPointerElementType() },
                        false
                ),
                &M
        );

        auto* load = new LoadInst(glob.getGlobal(), "", &Desc->front());
        auto* call = CallInst::Create(current, std::vector<llvm::Value*>{load}, "", &Desc->front());
        call->setMetadata("var", glob);
    }

    ReturnInst::Create(M.getContext(), &Desc->front());

    std::list<llvm::DbgValueInst*> toReplace;

    for (auto& F : M) {
        for (auto& BB : F) {
            for (auto& I : BB) {
                if (auto* call = dyn_cast<llvm::DbgValueInst>(&I)) {
                    toReplace.push_back(call);
                }
            }
        }
    }

    for (auto* call : toReplace) {
        auto* var = call->getVariable();
        auto* offset = call->getArgOperand(1);
        auto* val = call->getValue();

        auto* tp = llvm::FunctionType::get(
            llvm::Type::getVoidTy(M.getContext()),
            std::vector<llvm::Type*>{ offset->getType(), val->getType() },
            false
        );

        llvm::Function* intr = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_VALUE,
            toString(*val->getType()),
            tp,
            &M
        );

        auto* newCall = CallInst::Create(
            intr,
            std::vector<llvm::Value*>{ offset, val },
            "",
            call
        );
        newCall->setDebugLoc(call->getDebugLoc());
        newCall->setMetadata("dbg", call->getMetadata("dbg"));
        newCall->setMetadata("var", var);

        call->replaceAllUsesWith(newCall);
        call->eraseFromParent();
    }

    return false;
}

char MetaInserter::ID;
static RegisterPass<MetaInserter>
X("meta-inserter", "Replace all llvm.dbg.value calls with borealis.value.*");

} /* namespace borealis */
