/*
 * DataStructureAA.h
 *
 *  Created on: Sep 5, 2012
 *      Author: belyaev
 */

#ifndef DATASTRUCTUREAA_H_
#define DATASTRUCTUREAA_H_

#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Module.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/Passes.h>
#include "DataStructure.h"
#include "DSGraph.h"

namespace llvm {

class DSAA: public ModulePass, public AliasAnalysis {
	TDDataStructures *TD;
	BUDataStructures *BU;

	// These members are used to cache mod/ref information to make us return
	// results faster, particularly for aa-eval.  On the first request of
	// mod/ref information for a particular call site, we compute and store the
	// calculated nodemap for the call site.  Any time DSA info is updated we
	// free this information, and when we move onto a new call site, this
	// information is also freed.
	CallSite MapCS;
	std::multimap<DSNode*, const DSNode*> CallerCalleeMap;
public:
	static char ID;
	DSAA() :
		ModulePass(ID), TD(0) {
	}
	~DSAA() {
		InvalidateCache();
	}

	void InvalidateCache() {
		MapCS = CallSite();
		CallerCalleeMap.clear();
	}

	//------------------------------------------------
	// Implement the Pass API
	//

	// run - Build up the result graph, representing the pointer graph for the
	// program.
	//
	bool runOnModule(Module &) {
		InitializeAliasAnalysis(this);
		TD = &getAnalysis<TDDataStructures>();
		BU = &getAnalysis<BUDataStructures>();
		return false;
	}

	virtual void getAnalysisUsage(AnalysisUsage &AU) const {
		AliasAnalysis::getAnalysisUsage(AU);
		AU.setPreservesAll();                         // Does not transform code
		AU.addRequiredTransitive<TDDataStructures>(); // Uses TD Datastructures
		AU.addRequiredTransitive<BUDataStructures>(); // Uses BU Datastructures
	}

	//------------------------------------------------
	// Implement the AliasAnalysis API
	//

	virtual AliasResult alias(const Location& LocA, const Location& LocB);

	ModRefResult getModRefInfo(ImmutableCallSite CS, const Location& Loc);
	virtual ModRefResult getModRefInfo(ImmutableCallSite CS1,
			ImmutableCallSite CS2) {
		return AliasAnalysis::getModRefInfo(CS1, CS2);
	}

	virtual void deleteValue(Value *V) {
		InvalidateCache();
		BU->deleteValue(V);
		TD->deleteValue(V);
	}

	virtual void copyValue(Value *From, Value *To) {
		if (From == To)
			return;
		InvalidateCache();
		BU->copyValue(From, To);
		TD->copyValue(From, To);
	}

private:
	DSGraph *getGraphForValue(const Value *V);
};

}




#endif /* DATASTRUCTUREAA_H_ */
