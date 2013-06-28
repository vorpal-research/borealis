/*
 * MetaInserter.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#include "Codegen/intrinsics_manager.h"
#include "Passes/Transform/MetaInserter.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

MetaInserter::MetaInserter() : ModulePass(ID) {}

MetaInserter::~MetaInserter() {}

bool MetaInserter::runOnModule(llvm::Module &M) {
    using namespace llvm;
    using borealis::util::toString;
    using borealis::util::view;
    using borealis::util::viewContainer;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    llvm::DebugInfoFinder dfi;
    dfi.processModule(M);

    auto* GDT = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
            "",
            llvm::FunctionType::get(llvm::Type::getVoidTy(M.getContext()), false),
            &M
    );
    BasicBlock::Create(M.getContext(), "bb", GDT);

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
                    std::vector<llvm::Type*>{ glob.getGlobal()->getType()->getPointerElementType() },
                    false
                ),
                &M
        );

        auto* load = new LoadInst(glob.getGlobal(), "", &GDT->front());
        auto* call = CallInst::Create(current, std::vector<llvm::Value*>{ load }, "", &GDT->front());
        call->setMetadata("var", glob);
    }

    ReturnInst::Create(M.getContext(), &GDT->front());

    std::list<llvm::DbgValueInst*> toReplace;

    for (auto& I : viewContainer(M).flatten().flatten()) {
        if (auto* call = dyn_cast<llvm::DbgValueInst>(&I)) {
            toReplace.push_back(call);
        }
    }

    for (auto* call : toReplace) {
        auto* var = call->getVariable();
        auto* offset = call->getArgOperand(1);
        auto* val = call->getValue();

        auto* ty = llvm::FunctionType::get(
            llvm::Type::getVoidTy(M.getContext()),
            std::vector<llvm::Type*>{ offset->getType(), val->getType() },
            false
        );

        auto* current = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_VALUE,
            toString(*val->getType()),
            ty,
            &M
        );

        auto* newCall = CallInst::Create(
            current,
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
