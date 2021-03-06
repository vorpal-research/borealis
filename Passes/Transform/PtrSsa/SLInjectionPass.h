/*
 * SLInjectionPass.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef SLINJECTIONPASS_H_
#define SLINJECTIONPASS_H_

#include <llvm/IR/Dominators.h>
#include <llvm/Pass.h>

#include "Passes/Transform/PtrSsa/origin_tracker.h"
#include "Passes/Util/ProxyFunctionPass.h"
#include "Util/passes.hpp"

namespace borealis {
namespace ptrssa {

class StoreLoadInjectionPass:
        public ProxyFunctionPass,
        public ShouldBeModularized,
        public origin_tracker {

public:

    static char ID;

    StoreLoadInjectionPass() : ProxyFunctionPass(ID) {};
    StoreLoadInjectionPass(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {};

    virtual bool runOnFunction(llvm::Function& F) override {
        for (auto& bb : F) {
            runOnBasicBlock(bb);
        }
        return false;
    }
    bool runOnBasicBlock(llvm::BasicBlock& bb);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
    virtual ~StoreLoadInjectionPass() {}

private:

    llvm::Function* createNuevoFunc(llvm::Type* pointed, llvm::Module* daModule);

    void createNewDefs(llvm::BasicBlock& BB);
    void renameNewDefs(llvm::Instruction* newdef, llvm::Value* ptr);

    std::vector<llvm::Value*> getPointerOperands(llvm::StoreInst* store) {
        return std::vector<llvm::Value*>{ store->getPointerOperand() };
    }
    std::vector<llvm::Value*> getPointerOperands(llvm::LoadInst* load) {
        return std::vector<llvm::Value*>{ load->getPointerOperand() };
    }
    std::vector<llvm::Value*> getPointerOperands(llvm::CallInst* call) {
        if (call->doesNotAccessMemory()) return std::vector<llvm::Value*>();

        std::vector<llvm::Value*> ret;
        for (auto i = 0U; i < call->getNumArgOperands(); ++i) {
            auto* op = call->getArgOperand(i);
            if (op->getType()->isPointerTy()) ret.push_back(op);
        }
        return ret;
    }

    template<class InstType>
    inline void checkAndUpdatePtrs(llvm::Instruction* inst) {
        using namespace llvm;

        if (InstType* resolve = dyn_cast_or_null<InstType>(inst)) {
            auto* F = inst->getParent()->getParent();
            auto* M = F->getParent();
            auto ops = getPointerOperands(resolve);

            for (Value* op : ops) {
                if (!isa<Constant>(op)) {
                    std::string name;
                    if (Value* orig = getOrigin(op)) {
                        if (orig->hasName()) name = (orig->getName() + ".").str();
                    } else {
                        if (op->hasName()) name = (op->getName() + ".").str();
                    }

                    CallInst* newdef = CallInst::Create(
                        createNuevoFunc(op->getType()->getPointerElementType(), M),
                        op,
                        name,
                        inst
                    );
                    newdef->setDebugLoc(resolve->getDebugLoc());

                    renameNewDefs(newdef, op);

                    if (Value* orig = getOrigin(op)) {
                        setOrigin(newdef, orig);
                    } else {
                        setOrigin(newdef, op);
                    }
                }
            }
        }
    }

    llvm::DominatorTree* DT_;
};

} // namespace ptrssa
} // namespace borealis

#endif /* SLINJECTIONPASS_H_ */
