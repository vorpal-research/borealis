
#include "SLInjectionPass.h"

#include "PhiInjectionPass.h"

#include <iterator>

#include <llvm/Pass.h>
#include <llvm/BasicBlock.h>
#include <llvm/Function.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>



namespace {
using namespace borealis;
using namespace llvm;
}


static Function* createDummyPtrFunction(Type* pointed, Module* where) {
    auto ptr = PointerType::getUnqual(pointed);
    auto ftype = FunctionType::get(ptr, ptr, false /* isVarArg*/);
    return createIntrinsic(intrinsic::PTR_VERSION, where, ftype, pointed);
}

Function* ptrssa::StoreLoadInjectionPass::createNuevoFunc(
        Type* pointed, Module* daModule
) {
    if(nuevos.count(pointed)) {
        return nuevos[pointed];
    } else {
        return (nuevos[pointed] = createDummyPtrFunction(pointed, daModule));
    }
}

void ptrssa::StoreLoadInjectionPass::createNewDefs(BasicBlock &BB)
{
    using util::view;
    for (auto it = BB.begin(); it != BB.end() ; ++it) {
        checkAndUpdatePtrs<StoreInst>(it);
        checkAndUpdatePtrs<LoadInst>(it);
        checkAndUpdatePtrs<CallInst>(it);
    }
}

void ptrssa::StoreLoadInjectionPass::renameNewDefs(
        Instruction *newdef,
        Value *Op){
    // This vector of Instruction* points to the uses of V.
    // This auxiliary vector of pointers is used because the use_iterators
    // are invalidated when we do the renaming
    SmallVector<Instruction*, 25> usepointers;
    unsigned i = 0, n = Op->getNumUses();
    usepointers.resize(n);
    BasicBlock *BB = newdef->getParent();

    for (auto uit = Op->use_begin(), uend = Op->use_end(); uit != uend; ++uit, ++i)
        usepointers[i] = dyn_cast<Instruction>(*uit);

    for (i = 0; i < n; ++i) {
        if (usepointers[i] ==  NULL) {
            continue;
        }
        if (usepointers[i] == newdef) {
            continue;
        }

        BasicBlock *BB_user = usepointers[i]->getParent();
        BasicBlock::iterator newdefit(newdef);
        BasicBlock::iterator useit(usepointers[i]);

        // Check if the use is in the dominator tree of newdef(V)
        if (DT_->dominates(BB, BB_user)) {
            // If in the same basicblock, only rename if the use is posterior to the newdef
            if (BB_user == BB) {
                int dist1 = std::distance(BB->begin(), useit);
                int dist2 = std::distance(BB->begin(), newdefit);
                int offset = dist1 - dist2;

                if (offset > 0) {
                    usepointers[i]->replaceUsesOfWith(Op, newdef);
                }
            }
            else {
                usepointers[i]->replaceUsesOfWith(Op, newdef);
            }
        }
    }

}

void ptrssa::StoreLoadInjectionPass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.addRequiredTransitive<DominatorTree>();
    AU.addRequiredTransitive<SlotTrackerPass>();
    AU.addPreserved<PhiInjectionPass>();

    // This pass modifies the program, but not the CFG
    AU.setPreservesCFG();
    //AU.setPreservesAll(); // a lie
}

bool ptrssa::StoreLoadInjectionPass::runOnBasicBlock(BasicBlock& bb) {
    DT_ = &getAnalysis<DominatorTree>();

    auto phii = getAnalysisIfAvailable<PhiInjectionPass>();

    if(origins.empty() && phii) mergeOriginInfoFrom(*phii);
    createNewDefs(bb);
    return false;
}



char borealis::ptrssa::StoreLoadInjectionPass::ID;

namespace {
llvm::RegisterPass<borealis::ptrssa::StoreLoadInjectionPass> X("sl-injection",
        "Inject an intrinsic for every store and load");
}

