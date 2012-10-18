#define DEBUG_TYPE "uSSA"

#include "PtrSSAPass.h"
#include "llvm/Support/CommandLine.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/Casting.h>
using llvm::ArrayRef;

#include <regex>

using namespace llvm;
using namespace borealis;

#include "intrinsics.h"

static const std::string newdefstr = "_newdef";


void PtrSSAPass::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequiredTransitive<DominanceFrontier>();
	AU.addRequiredTransitive<DominatorTree>();
	AU.addRequiredTransitive<SlotTrackerPass>();
	
	// This pass modifies the program, but not the CFG
	AU.setPreservesCFG();
}

SlotTracker& PtrSSAPass::getSlotTracker(const Function* F) {
    return *getAnalysis<SlotTrackerPass>().getSlotTracker(F);
}

Value* PtrSSAPass::getOriginal(Value* v) {
    if(originals.count(v)) return originals[v];
    else return nullptr;
}
void PtrSSAPass::setOriginal(Value* v, Value* o) {
    originals[v] = o;
}

bool PtrSSAPass::runOnFunction(Function &F) {
	DT_ = &getAnalysis<DominatorTree>();
	DF_ = &getAnalysis<DominanceFrontier>();
	
	// Iterate over all Basic Blocks of the Function, calling the function that creates sigma functions, if needed
	for (auto& BB : F) {
		createNewDefs(BB);
	}
	
	return false;
}

static Function* createDummyPtrFunction(Type* pointed, Module* where) {
    auto ptr = PointerType::getUnqual(pointed);
    auto ftype = FunctionType::get(ptr, ptr, false /* isVarArg*/);
    return createIntrinsic(intrinsic::PTR_VERSION, where, ftype, pointed);
}

Function* PtrSSAPass::createNuevoFunc(Type* pointed, Module* daModule) {
    if(nuevo_funcs.count(pointed)) {
        return nuevo_funcs[pointed];
    } else {
        return (nuevo_funcs[pointed] = createDummyPtrFunction(pointed, daModule));
    }
}

template<class InstType>
inline static void CheckAndUpdatePtrs(PtrSSAPass* pass, Instruction* inst) {
    if(isa<InstType>(inst)) {
        auto resolve = dyn_cast<InstType>(inst);
        auto F = inst->getParent()->getParent();
        auto M = F->getParent();
        auto op = resolve->getPointerOperand();

        if(!isa<Constant>(op)) {
            string name;
            if(Value * orig = pass->getOriginal(op)) {
                if(orig->hasName()) name = orig->getName();
            } else {
                if(op->hasName()) name = op->getName();
            }

            CallInst *newdef = CallInst::Create(
                pass->createNuevoFunc(op->getType()->getPointerElementType(), M),
                ArrayRef<Value*>(op),
                name,
                resolve
            );
            newdef->setDebugLoc(resolve->getDebugLoc());

            pass->renameNewDefs(newdef, resolve, op);

            if(Value * orig = pass->getOriginal(op)) {
                pass->setOriginal(newdef, orig);
            } else {
                pass->setOriginal(newdef, op);
            }
        }
    }
}

static void propagatePHIS(
        PtrSSAPass* pass,
        BasicBlock& to,
        Instruction& what,
        DominatorTree* dt,
        DominanceFrontier* df,
        shared_ptr<map<BasicBlock*, PHINode*>> seen = nullptr) {
    using borealis::util::view;

    if(what.getParent() == &to) return;

    auto& from = *what.getParent();

    if(!seen) seen.reset(new map<BasicBlock*, PHINode*>());

    for(auto succ : view(succ_begin(&from), succ_end(&from))) {
        if(!seen->count(succ)) {
            PHINode* phi = nullptr;
            for(auto& psi: *succ) {
                if(!isa<PHINode>(psi)) break;
                auto& rephi = cast<PHINode>(psi);

                if(rephi.getIncomingValueForBlock(&from) == &what) phi = &rephi;
            }

            if(!phi) {
                string name = "";
                if(Value * orig = pass->getOriginal(&what)) {
                    if(orig->hasName()) name = orig->getName().str();
                } else {
                    if(what.hasName()) name = what.getName().str();
                }

                phi = PHINode::Create(what.getType(), 0, name, &succ->front());
                phi->addIncoming(&what, &from);

            }

            if(Value* orig = pass->getOriginal(&what)) pass->setOriginal(phi, orig);
            else pass->setOriginal(phi, &what);

            seen->insert(make_pair(succ, phi));

            errs() << "inserted:" << *phi << "\n";

            propagatePHIS(pass, to, *phi, dt, df, seen);
        } else {
            PHINode* phi = seen->at(succ);
            if(phi->getBasicBlockIndex(&from) == -1) {
                phi->addIncoming(&what, &from);
            }
        }
    }
}

static void mergePHIsIfPossible(PtrSSAPass* pass, PHINode* to, PHINode* from) {
    if(to == from) return;

    errs() << "Phi merging:" << "\n"
           << *to << '/' << *from << "\n";
    errs() << "origins:" << "\n";
    if(pass->getOriginal(to)) errs() << *pass->getOriginal(to);
    if(pass->getOriginal(from)) errs() << *pass->getOriginal(from);
    errs() << "----\n";

    if(
        pass->getOriginal(to) == pass->getOriginal(from) &&
        to->getParent() == from->getParent()
    ) {
        for(auto i = 0U; i < from->getNumIncomingValues(); ++i) {
            to->addIncoming(from->getIncomingValue(i), from->getIncomingBlock(i));
        }
        //from ->eraseFromParent();
    }
}

void PtrSSAPass::createNewDefs(BasicBlock &BB)
{
    using util::view;
    Module* M = BB.getParent()->getParent();
	for (auto it = BB.begin(); it != BB.end() ; ++it) {
	    CheckAndUpdatePtrs<StoreInst>(this, it);
	    CheckAndUpdatePtrs<LoadInst>(this, it);
	}
	for (auto it = BB.begin(); it != BB.end() ; ++it) {
	    for(auto i = 0UL; i < it->getNumOperands(); ++i) {
	        auto m = make_shared<map<BasicBlock*, PHINode*>>();

	        if(auto inst = dyn_cast<Instruction>(it->getOperand(i))) {
	            if(inst->getParent() == it->getParent()) continue;
	            if(!inst->getType()->isPointerTy()) continue;

	            propagatePHIS(this, BB, *inst, DT_, DF_, m);
	            if(m->count(&BB)) it->setOperand(i, m->at(&BB));

	            errs() << "--\n";
	            for(auto& p0 : *m) {
                    for(auto& p1 : *m) {
                        mergePHIsIfPossible(this, p0.second, p1.second);
                    }
	            }
	            for(auto& p0 : *m) {
	                errs() << *p0.second << ":" << p0.second->getNumUses() << "\n";
                    if(p0.second->getNumUses() == 0) {
                        p0.second->removeFromParent();
                    }
	            }
	        }
	    }
	}

}



void PtrSSAPass::renameNewDefs(Instruction *newdef, Instruction* V, Value* Op)
{
	// This vector of Instruction* points to the uses of V.
	// This auxiliary vector of pointers is used because the use_iterators are invalidated when we do the renaming
	SmallVector<Instruction*, 25> usepointers;
	unsigned i = 0, n = Op->getNumUses();
	usepointers.resize(n);
	BasicBlock *BB = newdef->getParent();
	
	for (Value::use_iterator uit = Op->use_begin(), uend = Op->use_end(); uit != uend; ++uit, ++i)
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

char PtrSSAPass::ID = 0;
static RegisterPass<PtrSSAPass> X("pointer-ssa",
        "The pass that places an intrinsic mark on every pointer before every use");
