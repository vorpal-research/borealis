/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "Passes/PredicateStateAnalysis.h"
#include "State/Transformer/CallSiteInitializer.h"
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
    init();

    FM = &GetAnalysis< FunctionManager >::doit(this, F);

    auto* ST = GetAnalysis< SlotTrackerPass >::doit(this, F).getSlotTracker(F);
    PF = PredicateFactory::get(ST);
    TF = TermFactory::get(ST);

    PSF = PredicateStateFactory::get();

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
    PredicateState::Ptr initialState =
            PSF->Basic() +
            gPredicate +
            FM->get(&F)->filterByTypes({ PredicateType::REQUIRES });

    // Register arguments as visited values
    for (auto& arg : F.getArgumentList()) {
        initialState = initialState << arg;
    }

    enqueue(nullptr, &F.getEntryBlock(), initialState);
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
        PredicateState::Ptr state) {
    TRACE_UP("psa::queue");
    workQueue.push(std::make_tuple(from, to, state));
}

void PredicateStateAnalysis::processQueue() {
    while (!workQueue.empty()) {
        processBasicBlock(workQueue.front());
        workQueue.pop();
        TRACE_DOWN("psa::queue");
    }
}

void PredicateStateAnalysis::processBasicBlock(const WorkQueueEntry& wqe) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::toString;
    using borealis::util::view;

    const BasicBlock* from = std::get<0>(wqe);
    const BasicBlock* bb = std::get<1>(wqe);
    PredicateState::Ptr inState = std::get<2>(wqe);

    // if (inState->isUnreachable()) return;

    inState = PSF->Chain(inState, PSF->Basic());

    auto iter = bb->begin();

    // Add incoming predicates from PHI nodes
    for ( ; isa<PHINode>(iter); ++iter) {
        const PHINode* phi = cast<PHINode>(iter);
        if (phi->getBasicBlockIndex(from) != -1) {
            inState = inState + PPM({from, phi}) << phi;
        }
    }

    for (auto& I : view(iter, bb->end())) {

        PredicateState::Ptr modifiedInState = inState + PM(&I) << I;
        predicateStateMap[&I] = predicateStateMap[&I].merge(modifiedInState);

        TRACE_MEASUREMENT(
                "psa::states." + toString(&I),
                predicateStateMap[&I].size(),
                "at",
                valueSummary(I));

        // Add ensures and summary *after* the CallInst has been processed
        if (isa<CallInst>(I)) {
            CallInst& CI = cast<CallInst>(I);

            PredicateState::Ptr callState = FM->get(CI, PF.get(), TF.get());
            CallSiteInitializer csi(CI, TF.get());

            PredicateState::Ptr transformedCallState = callState->filterByTypes(
                { PredicateType::ENSURES, PredicateType::STATE }
            )->map(
                [&csi](Predicate::Ptr p) {
                    return csi.transform(p);
                }
            );

            modifiedInState = modifiedInState + transformedCallState;
        }

        inState = modifiedInState;
    }

    processTerminator(*bb->getTerminator(), inState);
}

void PredicateStateAnalysis::processTerminator(
        const llvm::TerminatorInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    auto s = state << I;

    if (isa<BranchInst>(I))
    { processBranchInst(cast<BranchInst>(I), s); }
    else if (isa<SwitchInst>(I))
    { processSwitchInst(cast<SwitchInst>(I), s); }
}

void PredicateStateAnalysis::processBranchInst(
        const llvm::BranchInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        enqueue(I.getParent(), succ, state);
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        enqueue(I.getParent(), trueSucc, state + TPM({&I, trueSucc}));
        enqueue(I.getParent(), falseSucc, state + TPM({&I, falseSucc}));
    }
}

void PredicateStateAnalysis::processSwitchInst(
        const llvm::SwitchInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    for (auto c = I.case_begin(); c != I.case_end(); ++c) {
        const BasicBlock* caseSucc = c.getCaseSuccessor();
        enqueue(I.getParent(), caseSucc, state + TPM({&I, caseSucc}));
    }

    const BasicBlock* defaultSucc = I.getDefaultDest();
    enqueue(I.getParent(), defaultSucc, state + TPM({&I, defaultSucc}));
}

PredicateState::Ptr PredicateStateAnalysis::PM(const llvm::Instruction* I) {
    using borealis::util::containsKey;

    PredicateState::Ptr res = PSF->Basic();

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getPredicateMap();
        if (containsKey(map, I)) {
            res = res + map.at(I);
        }
    }

    return res;
}

PredicateState::Ptr PredicateStateAnalysis::PPM(PhiBranch key) {
    using borealis::util::containsKey;

    PredicateState::Ptr res = PSF->Basic();

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getPhiPredicateMap();
        if (containsKey(map, key)) {
            res = res + map.at(key);
        }
    }

    return res;
}

PredicateState::Ptr PredicateStateAnalysis::TPM(TerminatorBranch key) {
    using borealis::util::containsKey;

    PredicateState::Ptr res = PSF->Basic();

    for (AbstractPredicateAnalysis* APA : PA) {
        auto& map = APA->getTerminatorPredicateMap();
        if (containsKey(map, key)) {
            res = res + map.at(key);
        }
    }

    return res;
}

char PredicateStateAnalysis::ID;
static RegisterPass<PredicateStateAnalysis>
X("predicate-state-analysis", "Analysis that merges the results of separate predicate analyses");

} /* namespace borealis */
