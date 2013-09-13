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

llvm::Value* MetaInserter::liftDebugIntrinsic(llvm::Module& M, llvm::Value* ci) {
    using namespace borealis::util;
    using llvm::dyn_cast;
    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    if(auto* call = dyn_cast<llvm::DbgValueInst>(ci)) {
        auto* var = call->getVariable();
        auto* offset = call->getArgOperand(1);
        auto* val = call->getValue();

        llvm::Type* types[]{ offset->getType(), val->getType() };
        auto* ty = llvm::FunctionType::get(
            llvm::Type::getVoidTy(M.getContext()),
            llvm::makeArrayRef(types),
            false
        );

        auto* current = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_VALUE,
            toString(*val->getType()),
            ty,
            &M
        );

        llvm::Value* args[]{ offset, val };
        auto* newCall = llvm::CallInst::Create(
            current,
            llvm::makeArrayRef(args),
            "",
            call
        );
        newCall->setDebugLoc(call->getDebugLoc());
        newCall->setMetadata("dbg", call->getMetadata("dbg"));
        newCall->setMetadata("var", var);

        call->replaceAllUsesWith(newCall);
        call->eraseFromParent();
        return newCall;
    }

    if(auto* call = dyn_cast<llvm::DbgDeclareInst>(ci)) {
        auto* var = call->getVariable();
        auto* addr = call->getAddress();

        auto* ty = llvm::FunctionType::get(
            llvm::Type::getVoidTy(M.getContext()),
            addr->getType(),
            false
        );

        auto* current = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_VALUE,
            toString(*addr->getType()->getPointerElementType()),
            ty,
            &M
        );

        auto* newCall = llvm::CallInst::Create(
            current,
            addr,
            "",
            call
        );
        newCall->setDebugLoc(call->getDebugLoc());
        newCall->setMetadata("dbg", call->getMetadata("dbg"));
        newCall->setMetadata("var", var);

        call->replaceAllUsesWith(newCall);
        call->eraseFromParent();
        return newCall;
    }

    return ci;
}

llvm::Value* MetaInserter::unliftDebugIntrinsic(llvm::Module& M, llvm::Value* ci) {
    auto& intrinsic_manager = IntrinsicsManager::getInstance();
    using llvm::dyn_cast;

    if(auto* inst = dyn_cast<llvm::CallInst>(ci)) {
        if(intrinsic_manager.getIntrinsicType(*inst) == function_type::INTRINSIC_DECLARE) {
            auto* val = inst->getArgOperand(0);
            auto* varMD = inst->getMetadata("var");

            auto llvmIntr = llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::dbg_declare);

            llvm::Value *Args[] = { llvm::MDNode::get(M.getContext(), val), varMD };

            auto ret = llvm::CallInst::Create(llvmIntr, Args, "", inst);

            ret->setDebugLoc(inst->getDebugLoc());
            ret->setMetadata("dbg", inst->getMetadata("dbg"));

            inst->replaceAllUsesWith(ret);
            inst->eraseFromParent();
            return ret;
        }
        if(intrinsic_manager.getIntrinsicType(*inst) == function_type::INTRINSIC_VALUE) {
            auto* val = inst->getArgOperand(0);
            auto* offset = inst->getArgOperand(1);
            auto* varMD = inst->getMetadata("var");

            auto llvmIntr = llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::dbg_value);

            llvm::Value *Args[] = { llvm::MDNode::get(M.getContext(), val), offset, varMD };

            auto ret = llvm::CallInst::Create(llvmIntr, Args, "", inst);

            ret->setDebugLoc(inst->getDebugLoc());
            ret->setMetadata("dbg", inst->getMetadata("dbg"));

            inst->replaceAllUsesWith(ret);
            inst->eraseFromParent();
            return ret;
        }
    }

    return ci;
}

void MetaInserter::liftAllDebugIntrinsics(llvm::Module& M) {
    using llvm::dyn_cast;
    using namespace borealis::util;
    auto dyn_caster = [](llvm::Value& v){ return dyn_cast<llvm::DbgInfoIntrinsic>(&v); };

    auto view = viewContainer(M).
            flatten().
            flatten().
            map(dyn_caster).
            filter().
            to<std::list<llvm::Value*>>();

    for(auto I : view) {
        liftDebugIntrinsic(M, I);
    }
}

void MetaInserter::unliftAllDebugIntrinsics(llvm::Module& M) {
    using llvm::dyn_cast;
    using namespace borealis::util;
    auto dyn_caster = [](llvm::Value& v){ return dyn_cast<llvm::CallInst>(&v); };

    auto view = viewContainer(M).
            flatten().
            flatten().
            map(dyn_caster).
            filter().
            to<std::list<llvm::Value*>>();

    for(auto I : view) {
        unliftDebugIntrinsic(M, I);
    }
}

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
                    llvm::makeArrayRef(glob.getGlobal()->getType()->getPointerElementType()),
                    false
                ),
                &M
        );

        auto* load = new LoadInst(glob.getGlobal(), "", &GDT->front());
        auto* call = CallInst::Create(current, std::vector<llvm::Value*>{ load }, "", &GDT->front());
        call->setMetadata("var", glob);
    }

    ReturnInst::Create(M.getContext(), &GDT->front());

    std::list<llvm::DbgInfoIntrinsic*> toReplace;

    for (auto& I : viewContainer(M).flatten().flatten()) {
        if (auto* call = dyn_cast<llvm::DbgInfoIntrinsic>(&I)) {
            toReplace.push_back(call);
        }
    }

    // lift all llvm.dbg.
    for (auto call : toReplace) liftDebugIntrinsic(M, call);

    return false;
}

char MetaInserter::ID;
static RegisterPass<MetaInserter>
X("meta-inserter", "Replace all llvm.dbg.value calls with borealis.value.*");

} /* namespace borealis */
