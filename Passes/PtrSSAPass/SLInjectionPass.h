/*
 * SLInjectionPass.h
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#ifndef SLINJECTIONPASS_H_
#define SLINJECTIONPASS_H_

#include <llvm/Pass.h>
#include <llvm/Value.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Analysis/Dominators.h>

#include <unordered_map>

#include "intrinsics.h"

#include "origin_tracker.h"
#include "../SlotTrackerPass.h"

#include "../ProxyFunctionPass.hpp"

namespace borealis {
namespace ptrssa {

namespace {
    using llvm::FunctionPass;
    using llvm::BasicBlockPass;
    using llvm::RegisterPass;
    using llvm::BasicBlock;
    using llvm::Function;
    using llvm::Module;
    using llvm::Value;
    using llvm::Constant;
    using llvm::Type;
    using llvm::Instruction;
    using llvm::isa;
    using llvm::CallInst;
    using llvm::AnalysisUsage;
    using llvm::dyn_cast;
}

class StoreLoadInjectionPass:
        public ProxyFunctionPass<StoreLoadInjectionPass>,
        public origin_tracker {
    typedef ProxyFunctionPass<StoreLoadInjectionPass> base;
public:
    static char ID;
    typedef llvm::Value* value;

    StoreLoadInjectionPass(): base(), DT_(nullptr) {};
    StoreLoadInjectionPass(FunctionPass* del): base(del), DT_(nullptr) {};

    virtual bool runOnFunction(Function& F) {
        for(auto& bb: F) {
            runOnBasicBlock(bb);
        }
        return false;
    }
    virtual bool runOnBasicBlock(BasicBlock& bb);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

    virtual ~StoreLoadInjectionPass(){}


private:
    Function* createNuevoFunc(Type* pointed, Module* daModule);

    void createNewDefs(BasicBlock &BB);
    void renameNewDefs(Instruction *newdef, Instruction* olddef, Value* ptr);

    std::vector<llvm::Value*> getPointerOperands(StoreInst* store) {
        return std::vector<llvm::Value*>{ store->getPointerOperand() };
    }

    std::vector<llvm::Value*> getPointerOperands(LoadInst* load) {
        return std::vector<llvm::Value*>{ load->getPointerOperand() };
    }

    std::vector<llvm::Value*> getPointerOperands(CallInst* call) {

        if(call->doesNotAccessMemory()) return std::vector<llvm::Value*>();

        std::vector<llvm::Value*> ret;

        for(auto i = 0U; i< call->getNumArgOperands(); ++i) {
            auto op = call->getArgOperand(i);
            errs() << *op << endl;
            if(op->getType()->isPointerTy()) ret.push_back(op);
        }
        return ret;
    }

    template<class InstType>
    inline void checkAndUpdatePtrs(Instruction* inst) {
        if(isa<InstType>(inst)) {
            auto resolve = dyn_cast<InstType>(inst);
            auto F = inst->getParent()->getParent();
            auto M = F->getParent();
            auto ops = getPointerOperands(resolve);

            for(Value* op: ops) {
                if(!isa<Constant>(op)) {
                    string name;

                    if(Value * orig = getOrigin(op)) {
                        if(orig->hasName()) name = (orig->getName() + ".").str();
                    } else {
                        if(op->hasName()) name = (op->getName() + ".").str();
                    }



                    CallInst *newdef = CallInst::Create(
                        createNuevoFunc(op->getType()->getPointerElementType(), M),
                        op,
                        name,
                        inst
                    );
                    newdef->setDebugLoc(resolve->getDebugLoc());

                    renameNewDefs(newdef, resolve, op);

                    if(Value * orig = getOrigin(op)) {
                        setOrigin(newdef, orig);
                    } else {
                        setOrigin(newdef, op);
                    }
                }
            }
        }
    }

    typed_intrinsics_cache nuevos;

    llvm::DominatorTree* DT_;
};


}
}






#endif /* SLINJECTIONPASS_H_ */
