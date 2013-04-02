/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "Passes/PredicateStateAnalysis.h"
#include "State/CallSiteInitializer.h"
#include "Util/util.h"

// Hacky-hacky way to include all predicate analyses
#include "Passes/PredicateAnalysis.def"

namespace borealis {

PredicateStateAnalysis::PredicateStateAnalysis() :
        ProxyFunctionPass(ID) {}
PredicateStateAnalysis::PredicateStateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    using namespace llvm;

    AU.setPreservesAll();

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);

#define HANDLE_ANALYSIS(CLASS) \
    AUX<CLASS>::addRequiredTransitive(AU);
#include "Passes/PredicateAnalysis.def"
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    using namespace llvm;

    TRACE_FUNC;

    init();

    FM = &GetAnalysis< FunctionManager >::doit(this, F);

    auto* ST = GetAnalysis< SlotTrackerPass >::doit(this, F).getSlotTracker(F);
    PF = PredicateFactory::get(ST);
    TF = TermFactory::get(ST);

#define HANDLE_ANALYSIS(CLASS) \
    PA.push_back(static_cast<AbstractPredicateAnalysis*>(&GetAnalysis<CLASS>::doit(this, F)));
#include "Passes/PredicateAnalysis.def"

    // Register globals in our predicate states
    std::vector<Term::Ptr> globals;
    globals.reserve(F.getParent()->getGlobalList().size());
    for (auto& g : F.getParent()->getGlobalList()) {
        globals.push_back(TF->getValueTerm(&g));
    }
    Predicate::Ptr gPredicate = PF->getGlobalsPredicate(globals);

    // Register REQUIRES from annotations
    PredicateState requires = FM->get(&F).filterByTypes({ PredicateType::REQUIRES });

    enqueue(nullptr, &F.getEntryBlock(), gPredicate + requires);
    processQueue();

    return false;
}

void PredicateStateAnalysis::print(llvm::raw_ostream&, const llvm::Module*) const {
    infos() << "Predicate state analysis results" << endl;
    for (auto& e : predicateStateMap) {
        infos() << *e.first << endl
                << e.second << endl;
    }
    infos() << "End of predicate state analysis results" << endl;
}

PredicateStateAnalysis::~PredicateStateAnalysis() {}

PredicateStateAnalysis::PredicateStateMap& PredicateStateAnalysis::getPredicateStateMap() {
    return predicateStateMap;
}

////////////////////////////////////////////////////////////////////////////////

void PredicateStateAnalysis::init() {
    predicateStateMap.clear();

    WorkQueue q;
    std::swap(workQueue, q);
}

void PredicateStateAnalysis::enqueue(
        const llvm::BasicBlock* from,
        const llvm::BasicBlock* to,
        PredicateState state) {
    workQueue.push(std::make_tuple(from, to, state));
}

void PredicateStateAnalysis::processQueue() {
    while (!workQueue.empty()) {
        processBasicBlock(workQueue.front());
        workQueue.pop();
    }
}

void PredicateStateAnalysis::processBasicBlock(const WorkQueueEntry& wqe) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::view;

    const BasicBlock* from = std::get<0>(wqe);
    const BasicBlock* bb = std::get<1>(wqe);
    PredicateState inState = std::get<2>(wqe);

    if (inState.isUnreachable()) return;

    auto iter = bb->begin();

    // Add incoming predicates from PHI nodes
    for ( ; isa<PHINode>(iter); ++iter) {
        const PHINode* phi = cast<PHINode>(iter);
        if (phi->getBasicBlockIndex(from) != -1) {
            inState = inState.addAll(PPM({from, phi})).addVisited(phi);
        }
    }

    for (auto& I : view(iter, bb->end())) {

        PredicateState modifiedInState = inState.addAll(PM(&I)).addVisited(&I);
        predicateStateMap[&I] = predicateStateMap[&I].merge(modifiedInState);

        // Add ENSURES *after* the CallInst has been processed
        if (isa<CallInst>(I)) {
            CallInst& CI = cast<CallInst>(I);

            const PredicateState& callState = FM->get(CI, PF.get(), TF.get());
            CallSiteInitializer csi(CI, TF.get());

            PredicateState transformedCallState = callState.filterByTypes(
                { PredicateType::ENSURES }
            ).map(
                [&csi](Predicate::Ptr p) {
                    return csi.transform(p);
                }
            );

            modifiedInState = modifiedInState.addAll(transformedCallState);
        }

        inState = modifiedInState;
    }

    processTerminator(*bb->getTerminator(), inState);
}

void PredicateStateAnalysis::processTerminator(
        const llvm::TerminatorInst& I,
        const PredicateState& state) {
    using namespace::llvm;

    auto s = state.addVisited(&I);

    if (isa<BranchInst>(I))
    { processBranchInst(cast<BranchInst>(I), s); }
    else if (isa<SwitchInst>(I))
    { processSwitchInst(cast<SwitchInst>(I), s); }
}

void PredicateStateAnalysis::processBranchInst(
        const llvm::BranchInst& I,
        const PredicateState& state) {
    using namespace::llvm;

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        enqueue(I.getParent(), succ, state);
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        const PredicateState trueState = TPM({&I, trueSucc});
        const PredicateState falseState = TPM({&I, falseSucc});

        enqueue(I.getParent(), trueSucc, state.addAll(trueState));
        enqueue(I.getParent(), falseSucc, state.addAll(falseState));
    }
}

void PredicateStateAnalysis::processSwitchInst(
        const llvm::SwitchInst& I,
        const PredicateState& state) {
    using namespace::llvm;

    for (auto c = I.case_begin(); c != I.case_end(); ++c) {
        const BasicBlock* caseSucc = c.getCaseSuccessor();
        const PredicateState caseState = TPM({&I, caseSucc});
        enqueue(I.getParent(), caseSucc, state.addAll(caseState));
    }

    const BasicBlock* defaultSucc = I.getDefaultDest();
    const PredicateState defaultState = TPM({&I, defaultSucc});
    enqueue(I.getParent(), defaultSucc, state.addAll(defaultState));
}

PredicateState PredicateStateAnalysis::PM(const llvm::Instruction* I) {
    using borealis::util::containsKey;

    PredicateState res;

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getPredicateMap();
        if (containsKey(map, I)) {
            res = res.addPredicate(map.at(I));
        }
    }

    return res;
}

PredicateState PredicateStateAnalysis::PPM(PhiBranch key) {
    using borealis::util::containsKey;

    PredicateState res;

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getPhiPredicateMap();
        if (containsKey(map, key)) {
            res = res.addPredicate(map.at(key));
        }
    }

    return res;
}

PredicateState PredicateStateAnalysis::TPM(TerminatorBranch key) {
    using borealis::util::containsKey;

    PredicateState res;

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getTerminatorPredicateMap();
        if (containsKey(map, key)) {
            res = res.addPredicate(map.at(key));
        }
    }

    return res;
}

char PredicateStateAnalysis::ID;
static RegisterPass<PredicateStateAnalysis>
X("predicate-state-analysis", "Analysis that merges the results of separate predicate analyses");

} /* namespace borealis */
