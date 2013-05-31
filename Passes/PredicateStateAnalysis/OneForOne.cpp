/*
 * OneForOne.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Logging/tracer.hpp"
#include "Passes/PredicateStateAnalysis/OneForOne.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/CallSiteInitializer.h"
#include "Util/util.h"

// Hacky-hacky way to include all predicate analyses
#include "Passes/PredicateAnalysis.def"

namespace borealis {

OneForOne::OneForOne() : ProxyFunctionPass(ID) {}
OneForOne::OneForOne(llvm::Pass* pass) : ProxyFunctionPass(ID, pass) {}

void OneForOne::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    using namespace llvm;

    AU.setPreservesAll();

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);

#define HANDLE_ANALYSIS(CLASS) \
    AUX<CLASS>::addRequiredTransitive(AU);
#include "Passes/PredicateAnalysis.def"
}

bool OneForOne::runOnFunction(llvm::Function& F) {
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
    PredicateState::Ptr requires = FM->get(&F)->filterByTypes({ PredicateType::REQUIRES });

    PredicateState::Ptr initialState = (PSF * gPredicate + requires)();

    // Register arguments as visited values
    for (auto& arg : F.getArgumentList()) {
        initialState = initialState << arg;
    }

    // Queue up entry basic block
    enqueue(nullptr, &F.getEntryBlock(), initialState);

    // Process basic blocks
    processQueue();

    // Finalize results (merge alternatives as PSChoice)
    finalize();

    return false;
}

void OneForOne::print(llvm::raw_ostream&, const llvm::Module*) const {
    infos() << "Predicate state analysis results" << endl;
    for (auto& e : predicateStates) {
        infos() << *e.first << endl
                << e.second << endl;
    }
    infos() << "End of predicate state analysis results" << endl;
}

////////////////////////////////////////////////////////////////////////////////

void OneForOne::init() {
    AbstractPredicateStateAnalysis::init();

    predicateStates.clear();

    WorkQueue q;
    std::swap(workQueue, q);
}

void OneForOne::enqueue(
        const llvm::BasicBlock* from,
        const llvm::BasicBlock* to,
        PredicateState::Ptr state) {
    TRACE_UP("psa::queue");
    workQueue.push(std::make_tuple(from, to, state));
}

void OneForOne::processQueue() {
    while (!workQueue.empty()) {
        processBasicBlock(workQueue.front());
        workQueue.pop();
        TRACE_DOWN("psa::queue");
    }
}

void OneForOne::finalize() {
    for (const auto& e : predicateStates) {
        instructionStates[e.first] = PSF->Choice(e.second);
    }
}

void OneForOne::processBasicBlock(const WorkQueueEntry& wqe) {
    using namespace llvm;
    using borealis::util::containsKey;
    using borealis::util::toString;
    using borealis::util::view;

    const BasicBlock* from = std::get<0>(wqe);
    const BasicBlock* bb = std::get<1>(wqe);
    PredicateState::Ptr inState = std::get<2>(wqe);

    if (PredicateStateAnalysis::CheckUnreachable() && inState->isUnreachable()) {
        return;
    }

    auto iter = bb->begin();

    // Add incoming predicates from PHI nodes
    for ( ; isa<PHINode>(iter); ++iter) {
        const PHINode* phi = cast<PHINode>(iter);
        if (phi->getBasicBlockIndex(from) != -1) {
            inState = (PSF * inState + PPM({from, phi}) << phi)();
        }
    }

    for (auto& I : view(iter, bb->end())) {

        PredicateState::Ptr modifiedInState = (PSF * inState + PM(&I) << I)();
        predicateStates[&I].push_back(modifiedInState);

        TRACE_MEASUREMENT(
                "psa::states." + toString(&I),
                predicateStates[&I].size(),
                "at",
                valueSummary(I));

        // Add ensures and summary *after* the CallInst has been processed
        if (isa<CallInst>(I)) {
            CallInst& CI = cast<CallInst>(I);

            PredicateState::Ptr callState = FM->get(CI, PF.get(), TF.get());
            CallSiteInitializer csi(CI, TF.get());

            PredicateState::Ptr csiCallState = callState->filterByTypes(
                { PredicateType::ENSURES, PredicateType::STATE }
            )->map(
                [&csi](Predicate::Ptr p) {
                    return csi.transform(p);
                }
            );

            modifiedInState = (PSF * modifiedInState + csiCallState)();
        }

        inState = modifiedInState;
    }

    processTerminator(*bb->getTerminator(), inState);
}

void OneForOne::processTerminator(
        const llvm::TerminatorInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    auto s = state << I;

    if (isa<BranchInst>(I))
    { processBranchInst(cast<BranchInst>(I), s); }
    else if (isa<SwitchInst>(I))
    { processSwitchInst(cast<SwitchInst>(I), s); }
}

void OneForOne::processBranchInst(
        const llvm::BranchInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        enqueue(I.getParent(), succ, state);
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        PredicateState::Ptr trueState = TPM({&I, trueSucc});
        PredicateState::Ptr falseState = TPM({&I, falseSucc});

        enqueue(I.getParent(), trueSucc, (PSF * state + trueState)());
        enqueue(I.getParent(), falseSucc, (PSF * state + falseState)());
    }
}

void OneForOne::processSwitchInst(
        const llvm::SwitchInst& I,
        PredicateState::Ptr state) {
    using namespace::llvm;

    for (auto c = I.case_begin(); c != I.case_end(); ++c) {
        const BasicBlock* caseSucc = c.getCaseSuccessor();
        PredicateState::Ptr caseState = TPM({&I, caseSucc});
        enqueue(I.getParent(), caseSucc, (PSF * state + caseState)());
    }

    const BasicBlock* defaultSucc = I.getDefaultDest();
    PredicateState::Ptr defaultState = TPM({&I, defaultSucc});
    enqueue(I.getParent(), defaultSucc, (PSF * state + defaultState)());
}

PredicateState::Ptr OneForOne::PM(const llvm::Instruction* I) {
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

PredicateState::Ptr OneForOne::PPM(PhiBranch key) {
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

PredicateState::Ptr OneForOne::TPM(TerminatorBranch key) {
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

////////////////////////////////////////////////////////////////////////////////

char OneForOne::ID;
static RegisterPass<OneForOne>
X("one-for-one", "One-for-one predicate state analysis");

} /* namespace borealis */
