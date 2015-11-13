//
// Created by ice-phoenix on 10/13/15.
//

#include <llvm/Transforms/Utils/Local.h>

#include "Codegen/intrinsics_manager.h"
#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Util/passes.hpp"
#include "Util/functional.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

class KillStructByValue : public llvm::FunctionPass {

public:

    static char ID;

    KillStructByValue() : llvm::FunctionPass(ID) {}
    virtual ~KillStructByValue() {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        // Does not preserve CFG
        AU.setPreservesCFG();
    }

    virtual bool runOnFunction(llvm::Function& F) override {
        auto toReplace =
            util::viewContainer(F)
            .flatten()
            .filter(LAM(I, I.getType()->isStructTy()))
            .map(ops::take_pointer)
            .toVector();

        for(auto&& A: toReplace) {
            llvm::DemoteRegToStack(*A);
        }

        auto extracts =
            util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .map(llvm::ops::dyn_cast<llvm::ExtractValueInst>)
            .filter()
            .toVector();

        auto&& i32 = llvm::Type::getInt32Ty(F.getContext());

        // Extract(Load($ptr), $indices) -> Load(Gep($ptr, $indices))

        for(llvm::ExtractValueInst* E: extracts) {
            if(auto&& param = dyn_cast<llvm::LoadInst>(E->getAggregateOperand())) {
                auto indices = (util::itemize(0) >> util::viewContainer(E->getIndices()))
                               .map(LAM(Ix, static_cast<llvm::Value*>(llvm::ConstantInt::get(i32, Ix))))
                               .toVector();

                auto&& gep = llvm::GetElementPtrInst::Create(param->getPointerOperand(), indices, E->getName() + ".ptr", E);
                auto&& load = new llvm::LoadInst(gep, E->getName(), false, E);

                if(auto dbg = E->getMetadata("dbg")) {
                    gep->setMetadata("dbg", dbg);
                    load->setMetadata("dbg", dbg);
                }

                E->replaceAllUsesWith(load);
                E->eraseFromParent();

                if(param->getNumUses() == 0) param->eraseFromParent();
            }
        }

        auto inserts =
            util::viewContainer(F)
            .flatten()
            .map(ops::take_pointer)
            .map(llvm::ops::dyn_cast<llvm::StoreInst>)
            .filter()
            .filter(LAM(store, llvm::isa<llvm::InsertValueInst>(store->getValueOperand())))
            .toVector();

        // Store($ptr, InsertValue(Load($ptr), $indices, $value)) -> Store(Gep($ptr, $indices), $value)

        for(auto&& store : inserts) {
            auto&& insert = llvm::cast<llvm::InsertValueInst>(store->getValueOperand());
            if(auto&& load = dyn_cast<llvm::LoadInst>(insert->getAggregateOperand())) {
                auto indices = (util::itemize(0) >> util::viewContainer(insert->getIndices()))
                              .map(LAM(Ix, static_cast<llvm::Value*>(llvm::ConstantInt::get(i32, Ix))))
                              .toVector();

                auto&& ptr = store->getPointerOperand();
                auto&& ptr2 = load->getPointerOperand();
                if(ptr != ptr2) continue;

                auto&& value = insert->getInsertedValueOperand();

                auto&& gep = llvm::GetElementPtrInst::Create(ptr, indices, insert->getName() + ".ptr", store);
                auto&& newStore = new llvm::StoreInst(gep, value, false, store);

                if(auto&& dbg = store->getMetadata("dbg")) {
                    newStore->setMetadata("dbg", dbg);
                    gep->setMetadata("dbg", dbg);
                }

                store->replaceAllUsesWith(newStore);
                store->eraseFromParent();

                if(insert->getNumUses() == 0) insert->eraseFromParent();
                if(load->getNumUses() == 0) load->eraseFromParent();
            }
        }

        return not toReplace.empty();
    }

};

char KillStructByValue::ID;
static RegisterPass<KillStructByValue>
    X("kill-struct-values", "Erase insertValue/extractValue instructions in code");

} /* namespace borealis */

#include "Util/unmacros.h"
