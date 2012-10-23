/*
 *	This pass creates new definitions for variables used as operands
 *	of specifific instructions: add, sub, mul, trunc.
 *
 *	These new definitions are inserted right after the use site, and
 *	all remaining uses dominated by this new definition are renamed
 *	properly.
*/


#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Constants.h"
#include <deque>
#include <algorithm>
#include <unordered_map>

#include "SlotTrackerPass.h"

namespace borealis {

using namespace llvm;
using namespace std;

class PtrSSAPass : public FunctionPass {
public:
	static char ID; // Pass identification, replacement for typeid.
	PtrSSAPass() : FunctionPass(ID), DT_(nullptr), DF_(nullptr) {}
	void getAnalysisUsage(AnalysisUsage &AU) const;
	bool runOnFunction(Function&);
	
	SlotTracker& getSlotTracker(const Function*);
	Value* getOriginal(Value*);
	void setOriginal(Value*, Value*);

	Function* createNuevoFunc(Type* pointed, Module* daModule);

	void createNewDefs(BasicBlock &BB);
	void renameNewDefs(Instruction *newdef, Instruction* olddef, Value* ptr);

private:
	unordered_map<Type*, Function*> nuevo_funcs;
	unordered_map<Value*, Value*> originals;

	// Variables always live
	DominatorTree *DT_;
	DominanceFrontier *DF_;
};

}

bool getTruncInstrumentation();
