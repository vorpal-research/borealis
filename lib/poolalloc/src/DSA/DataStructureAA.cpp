//===- DataStructureAA.cpp - Data Structure Based Alias Analysis ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass uses the top-down data structure graphs to implement a simple
// context sensitive alias analysis.
//
//===----------------------------------------------------------------------===//


#include "DataStructureAA.h"
using namespace llvm;

char DSAA::ID = 0;

namespace {
// Register the pass...
RegisterPass<DSAA> X("ds-aa", "Data Structure Graph Based Alias Analysis");
}

ModulePass *llvm::createDSAAPass() { return new DSAA(); }

// getGraphForValue - Return the DSGraph to use for queries about the specified
// value...
//
DSGraph *DSAA::getGraphForValue(const Value *V) {
  if (const Instruction *I = dyn_cast<Instruction>(V))
    return TD->getDSGraph(*I->getParent()->getParent());
  else if (const Argument *A = dyn_cast<Argument>(V))
    return TD->getDSGraph(*A->getParent());
  else if (const BasicBlock *BB = dyn_cast<BasicBlock>(V))
    return TD->getDSGraph(*BB->getParent());
  return 0;
}

AliasAnalysis::AliasResult DSAA::alias(const Location& LocA,
                                       const Location& LocB) {
	errs() << "Running DS_AA::alias\n";
	errs().flush();

	const Value *V1 = LocA.Ptr;
	unsigned long long V1Size = LocA.Size;
	const Value *V2 = LocB.Ptr;
	unsigned long long V2Size = LocB.Size;

	errs() << *V1 << "\n"; errs().flush();
	errs() << *V2 << "\n"; errs().flush();

  if (V1 == V2) return MustAlias;

  if(!V1->getType()->isPointerTy() || !V2->getType()->isPointerTy()) return NoAlias;

  DSGraph *G1 = getGraphForValue(V1);
  errs() << "Graph 1: " << G1 << "\n"; errs().flush();
  DSGraph *G2 = getGraphForValue(V2);
  errs() << "Graph 2: " << G2 << "\n"; errs().flush();

  assert((!G1 || !G2 || G1 == G2) && "Alias query for 2 different functions?");
  if(G1 && G2 && G1 != G2) errs() << "Alias query for 2 different functions?" << "\n"; errs().flush();

  // Get the graph to use...
  DSGraph* G = G1 ? G1 : (G2 ? G2 : TD->getGlobalsGraph());

  errs() << "Graph: " << G << "\n"; errs().flush();

  G->print(errs());

  const DSGraph::ScalarMapTy &GSM = G->getScalarMap();

  DSGraph::ScalarMapTy::const_iterator I = GSM.find(V1);
  if (I == GSM.end()) return NoAlias;

  DSGraph::ScalarMapTy::const_iterator J = GSM.find(V2);
  if (J == GSM.end()) return NoAlias;

  DSNode  *N1 = I->second.getNode(),  *N2 = J->second.getNode();
  unsigned O1 = I->second.getOffset(), O2 = J->second.getOffset();
  if (N1 == 0 || N2 == 0)
    // Can't tell whether anything aliases null.
    return AliasAnalysis::alias(V1, V1Size, V2, V2Size);

  // We can only make a judgment if one of the nodes is complete.
  if (N1->isCompleteNode() || N2->isCompleteNode()) {
    if (N1 != N2)
      return NoAlias;   // Completely different nodes.

    // See if they point to different offsets...  if so, we may be able to
    // determine that they do not alias...
    if (O1 != O2) {
      if (O2 < O1) {    // Ensure that O1 <= O2
        std::swap(V1, V2);
        std::swap(O1, O2);
        std::swap(V1Size, V2Size);
      }

      if (O1+V1Size <= O2)
        return NoAlias;
    }
  }

  // FIXME: we could improve on this by checking the globals graph for aliased
  // global queries...
  return AliasAnalysis::alias(V1, V1Size, V2, V2Size);
}

/// getModRefInfo - does a callsite modify or reference a value?
///
AliasAnalysis::ModRefResult
DSAA::getModRefInfo(ImmutableCallSite ICS, const Location& Loc) {
	// FIXME: this is black magic. Mikhail Belyaev
	CallSite CS(const_cast<Instruction*>(ICS.getInstruction()));
	// black magic ends here

	auto& P = Loc.Ptr;
	auto& Size = Loc.Size;
  DSNode *N = 0;
  // First step, check our cache.
  if (CS.getInstruction() == MapCS.getInstruction()) {
    {
      const Function& Caller = *CS.getInstruction()->getParent()->getParent();
      DSGraph* CallerTDGraph = TD->getDSGraph(Caller);

      // Figure out which node in the TD graph this pointer corresponds to.
      DSScalarMap &CallerSM = CallerTDGraph->getScalarMap();
      DSScalarMap::iterator NI = CallerSM.find(P);
      if (NI == CallerSM.end()) {
        InvalidateCache();
        return DSAA::getModRefInfo(CS, Loc);
      }
      N = NI->second.getNode();
    }

  HaveMappingInfo:
    assert(N && "Null pointer in scalar map??");

    typedef std::multimap<DSNode*, const DSNode*>::iterator NodeMapIt;
    std::pair<NodeMapIt, NodeMapIt> Range = CallerCalleeMap.equal_range(N);

    // Loop over all of the nodes in the callee that correspond to "N", keeping
    // track of aggregate mod/ref info.
    bool NeverReads = true, NeverWrites = true;
    for (; Range.first != Range.second; ++Range.first) {
      if (Range.first->second->isModifiedNode())
        NeverWrites = false;
      if (Range.first->second->isReadNode())
        NeverReads = false;
      if (NeverReads == false && NeverWrites == false)
        return AliasAnalysis::getModRefInfo(CS, P, Size);
    }

    ModRefResult Result = ModRef;
    if (NeverWrites)      // We proved it was not modified.
      Result = ModRefResult(Result & ~Mod);
    if (NeverReads)       // We proved it was not read.
      Result = ModRefResult(Result & ~Ref);

    return ModRefResult(Result & AliasAnalysis::getModRefInfo(CS, P, Size));
  }

  // Any cached info we have is for the wrong function.
  InvalidateCache();

  const Function* F = CS.getCalledFunction();

  if (!F) return AliasAnalysis::getModRefInfo(CS, Loc);

  if (F->isDeclaration()) {
    // If we are calling an external function, and if this global doesn't escape
    // the portion of the program we have analyzed, we can draw conclusions
    // based on whether the global escapes the program.
    const Function& Caller = *CS.getInstruction()->getParent()->getParent();
    DSGraph *G = TD->getDSGraph(Caller);
    DSScalarMap::iterator NI = G->getScalarMap().find(P);
    if (NI == G->getScalarMap().end()) {
      // If it wasn't in the local function graph, check the global graph.  This
      // can occur for globals who are locally reference but hoisted out to the
      // globals graph despite that.
      G = G->getGlobalsGraph();
      NI = G->getScalarMap().find(P);
      if (NI == G->getScalarMap().end())
        return AliasAnalysis::getModRefInfo(CS, P, Size);
    }

    // If we found a node and it's complete, it cannot be passed out to the
    // called function.
    if (NI->second.getNode()->isCompleteNode())
      return NoModRef;
    return AliasAnalysis::getModRefInfo(CS, Loc);
  }

  // Get the graphs for the callee and caller.  Note that we want the BU graph
  // for the callee because we don't want all caller's effects incorporated!
  const Function& Caller = *CS.getInstruction()->getParent()->getParent();
  DSGraph* CallerTDGraph = TD->getDSGraph(Caller);
  DSGraph* CalleeBUGraph = BU->getDSGraph(*F);

  // Figure out which node in the TD graph this pointer corresponds to.
  DSScalarMap &CallerSM = CallerTDGraph->getScalarMap();
  DSScalarMap::iterator NI = CallerSM.find(P);
  if (NI == CallerSM.end()) {
    ModRefResult Result = ModRef;
    if (isa<ConstantPointerNull>(P) || isa<UndefValue>(P))
      return NoModRef;                 // null is never modified :)
    else {
      assert(isa<GlobalVariable>(P) &&
    cast<GlobalVariable>(P)->getType()->getElementType()->isFirstClassType() &&
             "This isn't a global that DSA inconsiderately dropped "
             "from the graph?");

      DSGraph* GG = CallerTDGraph->getGlobalsGraph();
      DSScalarMap::iterator NI = GG->getScalarMap().find(P);
      if (NI != GG->getScalarMap().end() && !NI->second.isNull()) {
        // Otherwise, if the node is only M or R, return this.  This can be
        // useful for globals that should be marked const but are not.
        DSNode *N = NI->second.getNode();
        if (!N->isModifiedNode())
          Result = (ModRefResult)(Result & ~Mod);
        if (!N->isReadNode())
          Result = (ModRefResult)(Result & ~Ref);
      }
    }

    if (Result == NoModRef) return Result;
    return ModRefResult(Result & AliasAnalysis::getModRefInfo(CS, P, Size));
  }

  // Compute the mapping from nodes in the callee graph to the nodes in the
  // caller graph for this call site.
  DSGraph::NodeMapTy CalleeCallerMap;

  DSCallSite DSCS = CallerTDGraph->getDSCallSiteForCallSite(CS);
  CallerTDGraph->computeCalleeCallerMapping(DSCS, *F, *CalleeBUGraph,
                                            CalleeCallerMap);

  // Remember the mapping and the call site for future queries.
  MapCS = CS;

  // Invert the mapping into CalleeCallerInvMap.
  for (DSGraph::NodeMapTy::iterator I = CalleeCallerMap.begin(),
         E = CalleeCallerMap.end(); I != E; ++I)
    CallerCalleeMap.insert(std::make_pair(I->second.getNode(), I->first));

  N = NI->second.getNode();
  goto HaveMappingInfo;
}
