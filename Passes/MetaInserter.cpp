/*
 * MetaInserter.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#include "MetaInserter.h"

#include <llvm/Support/Casting.h>
#include <llvm/Support/InstIterator.h>

namespace borealis {

MetaInserter::MetaInserter(): ModulePass(ID) {}

MetaInserter::~MetaInserter() {}

bool MetaInserter::runOnModule(llvm::Module &M) {
    using llvm::inst_begin;
    using llvm::inst_end;
    using borealis::util::view;
    using llvm::dyn_cast;
    using borealis::util::toString;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    std::list<llvm::DbgValueInst*> toReplace;

    for(auto& F : M) {
        for(auto& BB: F) {
            for(auto& I : BB) {
                dbgs() << I << endl;
                if(auto* call = dyn_cast<llvm::DbgValueInst>(&I)) {
                    toReplace.push_back(call);
                }
            }
        }
    }

    for(auto* call: toReplace) {
        auto* var = call->getVariable();
        auto* offset = call->getArgOperand(1);
        auto* val = call->getValue();

        auto* tp = llvm::FunctionType::get(
            llvm::Type::getVoidTy(M.getContext()),
            std::vector<llvm::Type*>{
                offset->getType(),
                val->getType()
            },
            false
        );

        llvm::Function* intr = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_VALUE,
            toString(*val->getType()),
            tp,
            &M
        );

        auto* newCall = llvm::CallInst::Create(
            intr,
            std::vector<llvm::Value*> {
                offset,
                val
            },
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

char MetaInserter::ID = 0U;

static llvm::RegisterPass<MetaInserter>
X("meta-inserter", "Replace all llvm.dbg.value calls with borealis.value.*");

} /* namespace borealis */
