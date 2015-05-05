/*
 * MetaInserter.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: belyaev
 */

#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Passes/Transform/MetaInserter.h"
#include "Util/passes.hpp"
#include "Util/util.h"

#include "Util/macros.h"

#define ASSERTA(wha, mes) ASSERT(wha, #wha " in " + mes)

namespace borealis {

MetaInserter::MetaInserter() : ModulePass(ID) {}

MetaInserter::~MetaInserter() {}

static llvm::Value* mkBorealisValue(
        llvm::Module& M,
        llvm::Instruction* existingCall,
        llvm::MDNode* var,
        llvm::Value* offset,
        llvm::Value* val) {
    using llvm::dyn_cast;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    llvm::Type* types[]{ offset->getType(), val->getType() };
    auto* ty = llvm::FunctionType::get(
        llvm::Type::getVoidTy(M.getContext()),
        llvm::makeArrayRef(types),
        false
    );

    auto* current = intrinsic_manager.createIntrinsic(
        function_type::INTRINSIC_VALUE,
        util::toString(*val->getType()),
        ty,
        &M
    );

    llvm::Value* args[]{ offset, val };
    auto* newCall = llvm::CallInst::Create(
        current,
        llvm::makeArrayRef(args),
        "",
        existingCall
    );
    newCall->setDebugLoc(existingCall->getDebugLoc());
    newCall->setMetadata("dbg", existingCall->getMetadata("dbg"));
    newCall->setMetadata("var", var);

    existingCall->replaceAllUsesWith(newCall);
    existingCall->eraseFromParent();
    return newCall;
}

static llvm::Value* mkBorealisDeclare(
        llvm::Module& M,
        llvm::Instruction* existingCall,
        llvm::MDNode* var,
        llvm::Value* addr) {
    using llvm::dyn_cast;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    auto* ty = llvm::FunctionType::get(
        llvm::Type::getVoidTy(M.getContext()),
        addr->getType(),
        false
    );

    auto* current = intrinsic_manager.createIntrinsic(
        function_type::INTRINSIC_DECLARE,
        util::toString(*addr->getType()->getPointerElementType()),
        ty,
        &M
    );

    auto* newCall = llvm::CallInst::Create(
        current,
        addr,
        "",
        existingCall
    );
    newCall->setDebugLoc(existingCall->getDebugLoc());
    newCall->setMetadata("dbg", existingCall->getMetadata("dbg"));
    newCall->setMetadata("var", var);

    existingCall->replaceAllUsesWith(newCall);
    existingCall->eraseFromParent();
    return newCall;
}

llvm::Value* MetaInserter::liftDebugIntrinsic(llvm::Module& M, llvm::Value* ci) {
    using llvm::dyn_cast;

    if (auto* call = dyn_cast<llvm::DbgValueInst>(ci)) {
        auto* var = call->getVariable();
        auto* offset = call->getArgOperand(1);
        auto* val = call->getValue();

        ASSERTA(var != nullptr, util::toString(*call));
        ASSERTA(offset != nullptr, util::toString(*call));

        if (!val) { // the value has been optimized out
            return mkBorealisValue(M, call, var, offset,
                llvm::UndefValue::get(
                    llvm::Type::getInt64Ty(M.getContext())
                ) // FIXME: What's the undef type used for?
            );
        }

        return mkBorealisValue(M, call, var, offset, val);
    }

    if (auto* call = dyn_cast<llvm::DbgDeclareInst>(ci)) {
        auto* var = call->getVariable();
        auto* addr = call->getAddress();

        ASSERTA(var != nullptr, util::toString(*call))

        if (!addr) { // the value has been optimized out
            return mkBorealisValue(M, call, var,
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(M.getContext()), 0, false),
                llvm::UndefValue::get(
                    llvm::Type::getInt64Ty(M.getContext())
                ) // FIXME: What's the undef type used for?
            );
        }

        return mkBorealisDeclare(M, call, var, addr);
    }

    return ci;
}

llvm::Value* MetaInserter::unliftDebugIntrinsic(llvm::Module& M, llvm::Value* ci) {
    using llvm::dyn_cast;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    if (auto* inst = dyn_cast<llvm::CallInst>(ci)) {
        switch (intrinsic_manager.getIntrinsicType(*inst)) {
        case function_type::INTRINSIC_DECLARE: {
            auto* val = inst->getArgOperand(0);
            auto* varMD = inst->getMetadata("var");

            auto llvmIntr = llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::dbg_declare);

            llvm::Value* args[] = { llvm::MDNode::get(M.getContext(), val), varMD };

            auto ret = llvm::CallInst::Create(llvmIntr, args, "", inst);

            ret->setDebugLoc(inst->getDebugLoc());
            ret->setMetadata("dbg", inst->getMetadata("dbg"));

            inst->replaceAllUsesWith(ret);
            inst->eraseFromParent();
            return ret;
        }
        case function_type::INTRINSIC_VALUE: {
            auto* val = inst->getArgOperand(1);
            auto* offset = inst->getArgOperand(0);
            auto* varMD = inst->getMetadata("var");

            auto llvmIntr = llvm::Intrinsic::getDeclaration(&M, llvm::Intrinsic::dbg_value);

            llvm::Value* args[] = { llvm::MDNode::get(M.getContext(), val), offset, varMD };

            auto ret = llvm::CallInst::Create(llvmIntr, args, "", inst);

            ret->setDebugLoc(inst->getDebugLoc());
            ret->setMetadata("dbg", inst->getMetadata("dbg"));

            inst->replaceAllUsesWith(ret);
            inst->eraseFromParent();
            return ret;
        }
        default: break;
        }
    }

    return ci;
}

void MetaInserter::liftAllDebugIntrinsics(llvm::Module& M) {
    using namespace borealis::util;
    using llvm::dyn_cast;

    auto dyn_caster = [](llvm::Value& v){ return dyn_cast<llvm::DbgInfoIntrinsic>(&v); };

    auto view = viewContainer(M).
            flatten().
            flatten().
            map(dyn_caster).
            filter().
            to<std::list<llvm::Value*>>();

    for (auto* I : view) {
        liftDebugIntrinsic(M, I);
    }
}

void MetaInserter::unliftAllDebugIntrinsics(llvm::Module& M) {
    using namespace borealis::util;
    using llvm::dyn_cast;

    auto dyn_caster = [](llvm::Value& v){ return dyn_cast<llvm::CallInst>(&v); };

    auto view = viewContainer(M).
            flatten().
            flatten().
            map(dyn_caster).
            filter().
            to<std::list<llvm::Value*>>();

    for (auto* I : view) {
        unliftDebugIntrinsic(M, I);
    }
}

bool MetaInserter::runOnModule(llvm::Module &M) {
    using namespace borealis::util;
    using namespace llvm;

    auto& intrinsic_manager = IntrinsicsManager::getInstance();

    borealis::DebugInfoFinder dfi;
    dfi.processModule(M);

    auto* GDT = intrinsic_manager.createIntrinsic(
            function_type::INTRINSIC_GLOBAL_DESCRIPTOR_TABLE,
            "",
            llvm::FunctionType::get(llvm::Type::getVoidTy(M.getContext()), false),
            &M
    );
    BasicBlock::Create(M.getContext(), "bb", GDT);

    for (auto& mglob : viewContainer(dfi.global_variables())) {
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
    for (auto* call : toReplace) liftDebugIntrinsic(M, call);

    return false;
}

char MetaInserter::ID;
static RegisterPass<MetaInserter>
X("meta-inserter", "Replace all llvm.dbg.* calls with borealis.*");

} /* namespace borealis */

#include "Util/unmacros.h"
