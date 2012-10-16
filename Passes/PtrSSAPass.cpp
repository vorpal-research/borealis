#define DEBUG_TYPE "uSSA"

#include "PtrSSAPass.h"
#include "llvm/Support/CommandLine.h"

#include <llvm/ADT/ArrayRef.h>
using llvm::ArrayRef;

using namespace llvm;
using namespace borealis;

static const std::string newdefstr = "_newdef";

void PtrSSAPass::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequiredTransitive<DominanceFrontier>();
	AU.addRequiredTransitive<DominatorTree>();
	
	// This pass modifies the program, but not the CFG
	AU.setPreservesCFG();
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
    std::string buf;
    llvm::raw_string_ostream funcname(buf);
    funcname << "borealis.nuevo." << *pointed;
    auto ptr = PointerType::getUnqual(pointed);
    auto args = ArrayRef<Type*>(ptr);
    auto ftype = FunctionType::get(ptr, args, false /* isVarArg*/);
    return Function::Create( ftype, GlobalValue::ExternalLinkage, Twine(funcname.str()), where );
}

Function* PtrSSAPass::createNuevoFunc(Type* pointed, Module* daModule) {
    if(nuevo_funcs.count(pointed)) {
        return nuevo_funcs[pointed];
    } else {
        return (nuevo_funcs[pointed] = createDummyPtrFunction(pointed, daModule));
    }
}

template<class InstType>
inline static void CheckAndUpdate(PtrSSAPass* pass, Instruction* inst) {
    if(isa<InstType>(inst)) {
        auto resolve = dyn_cast<InstType>(inst);
        Module* M = inst->getParent()->getParent()->getParent();

        auto op = resolve->getPointerOperand();
        if(!isa<Constant>(op)) {
            // FIXME: use slottracker here
            std::string newname = op->getName().str() + "$";

            errs() << *resolve << "\n"; errs().flush();
            errs() << *inst << "\n"; errs().flush();
            errs() << *op->getType() << "\n"; errs().flush();

            CallInst *newdef = CallInst::Create(
                pass->createNuevoFunc(op->getType()->getPointerElementType(), M),
                ArrayRef<Value*>(op),
                Twine(newname),
                resolve
            );
            newdef->setDebugLoc(resolve->getDebugLoc());

            pass->renameNewDefs(newdef, resolve, op);
        }
    }
}

void PtrSSAPass::createNewDefs(BasicBlock &BB)
{
    Module* M = BB.getParent()->getParent();
	for (auto it = BB.begin(); it != BB.end() ; ++it) {
	    CheckAndUpdate<StoreInst>(this, it);
	    CheckAndUpdate<LoadInst>(this, it);
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
	        errs() << *usepointers[i] << "\n"; errs().flush();
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
