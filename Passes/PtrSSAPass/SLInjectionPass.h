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


namespace borealis {
namespace ptrssa {

namespace {
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

class StoreLoadInjectionPass: public llvm::BasicBlockPass, public origin_tracker {
public:
    static char ID;
    typedef llvm::Value* value;

    StoreLoadInjectionPass(): BasicBlockPass(ID), DT_(nullptr) {};

    virtual bool runOnBasicBlock(BasicBlock& bb);

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;


private:
    Function* createNuevoFunc(Type* pointed, Module* daModule);

    void createNewDefs(BasicBlock &BB);
    void renameNewDefs(Instruction *newdef, Instruction* olddef, Value* ptr);

    template<class InstType>
    inline void checkAndUpdatePtrs(Instruction* inst) {
        if(isa<InstType>(inst)) {
            auto resolve = dyn_cast<InstType>(inst);
            auto F = inst->getParent()->getParent();
            auto M = F->getParent();
            auto op = resolve->getPointerOperand();

            if(!isa<Constant>(op)) {
                string name;
                if(Value * orig = getOrigin(op)) {
                    if(orig->hasName()) name = orig->getName();
                } else {
                    if(op->hasName()) name = op->getName();
                }

                CallInst *newdef = CallInst::Create(
                    createNuevoFunc(op->getType()->getPointerElementType(), M),
                    op,
                    name,
                    resolve
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

    typed_intrinsics_cache nuevos;

    llvm::DominatorTree* DT_;
};


}
}






#endif /* SLINJECTIONPASS_H_ */
