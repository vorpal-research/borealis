/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Feb 14, 2013
 *      Author: ice-phoenix
 */

#include "Config/config.h"
#include "Passes/PredicateStateAnalysis/Defines.def"
#include "Passes/PredicateStateAnalysis/PredicateStateAnalysis.h"
#include "Passes/Tracker/NameTracker.h"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "SMT/MathSAT/Solver.h"
#include "SMT/MathSAT/Unlogic/Unlogic.h"
#include "State/PredicateStateBuilder.h"
#include "State/Transformer/TermRebinder.h"

#include "Util/macros.h"

namespace borealis {

AbstractPredicateStateAnalysis::AbstractPredicateStateAnalysis() {};
AbstractPredicateStateAnalysis::~AbstractPredicateStateAnalysis() {};

////////////////////////////////////////////////////////////////////////////////

PredicateStateAnalysis::PredicateStateAnalysis() :
        ProxyFunctionPass(ID) {}
PredicateStateAnalysis::PredicateStateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    if ("one-for-one" == Mode())
        AUX<OneForOne>::addRequiredTransitive(AU);
    else if ("one-for-all" == Mode())
        AUX<OneForAll>::addRequiredTransitive(AU);
    else
        BYE_BYE_VOID("Unknown PSA mode: " + Mode());

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<LocationManager>::addRequiredTransitive(AU);
    AUX<NameTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    if ("one-for-one" == Mode())
        delegate = &GetAnalysis<OneForOne>::doit(this, F);
    else if ("one-for-all" == Mode())
        delegate = &GetAnalysis<OneForAll>::doit(this, F);
    else
        BYE_BYE(bool, "Unknown PSA mode: " + Mode());

    auto* st = GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F);
    FN = FactoryNest(st);

    if ("inline" == Summaries()) {
        updateInlineSummary(F);
    } else if ("interpol" == Summaries()) {
        updateInterpolSummary(F);
    }

    updateVisitedLocs(F);

    return false;
}

// Save total function state for inlining
void PredicateStateAnalysis::updateInlineSummary(llvm::Function& F) {

    auto& FM = GetAnalysis<FunctionManager>::doit(this);

    // No summary if:
    // - function does not return
    // - function returns void
    // - function returns non-pointer

    auto* RI = getSingleRetOpt(&F);
    // Function does not return, therefore has no useful summary
    if (!RI) return;

    auto initial = delegate->getInitialState();
    auto riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));

    auto bdy = riState->sliceOn(initial);
    ASSERT(bdy, "Function state slicing failed for: " + llvm::valueSummary(RI));

    FM.update(&F, bdy);
}

// Generate interpolation-based function summary
void PredicateStateAnalysis::updateInterpolSummary(llvm::Function& F) {

    using borealis::mathsat_::unlogic::undoThat;
    using borealis::util::view;

    USING_SMT_IMPL(MathSAT);

    auto& FM = GetAnalysis<FunctionManager>::doit(this);
    auto& NT = GetAnalysis<NameTracker>::doit(this);

    // No summary if:
    // - function does not return
    // - function returns void
    // - function returns non-pointer

    auto* RI = getSingleRetOpt(&F);
    if (!RI) return;

    auto* retVal = RI->getReturnValue();
    if (retVal == nullptr) return;
    if ( ! retVal->getType()->isPointerTy() ) return;

    std::vector<Term::Ptr> args;
    args.reserve(F.arg_size());
    for (auto& arg : view(F.arg_begin(), F.arg_end())) {
        args.push_back(FN.Term->getArgumentTerm(&arg));
    }

    PredicateState::Ptr query = (
        FN.State *
        FN.Predicate->getInequalityPredicate(
            FN.Term->getReturnValueTerm(&F),
            FN.Term->getNullPtrTerm()
        )
    )();

    auto initial = delegate->getInitialState();
    auto riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));

    auto bdy = riState->sliceOn(initial);
    ASSERT(bdy, "Function state slicing failed for: " + llvm::valueSummary(RI));

    dbgs() << "Generating summary: " << endl
           << "  At: " << llvm::valueSummary(RI) << endl
           << "  Query: " << query << endl
           << "  State: " << bdy << endl;

    ExprFactory ef;
    Solver s(ef);

    auto itp = s.getSummary(args, query, bdy);

    auto t = TermRebinder(F, &NT, FN);

    auto summ = undoThat(itp)->map(
        [&t](Predicate::Ptr p) { return t.transform(p); }
    );

    FM.update(&F, summ);
}

void PredicateStateAnalysis::updateVisitedLocs(llvm::Function& F) {
    auto& LM = GetAnalysis<LocationManager>::doit(this);

    auto* RI = getSingleRetOpt(&F);
    if (!RI) return;

    auto riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));

    LM.addLocations(riState->getVisited());
}

void PredicateStateAnalysis::print(llvm::raw_ostream& O, const llvm::Module* M) const {
    delegate->printInstructionStates(O, M);
}

PredicateState::Ptr PredicateStateAnalysis::getInstructionState(const llvm::Instruction* I) const {
    return delegate->getInstructionState(I);
}

////////////////////////////////////////////////////////////////////////////////

char PredicateStateAnalysis::ID;
static RegisterPass<PredicateStateAnalysis>
X("predicate-state-analysis", "Delegating analysis that merges the results of separate predicate analyses");

const std::string PredicateStateAnalysis::Mode() {
    static config::ConfigEntry<std::string> mode("analysis", "psa-mode");
    return mode.get("one-for-one");
}

bool PredicateStateAnalysis::CheckUnreachable() {
    static config::ConfigEntry<bool> checkUnreachable("analysis", "psa-check-unreachable");
    return checkUnreachable.get(true);
}

const std::string PredicateStateAnalysis::Summaries() {
    static config::ConfigEntry<std::string> mode("analysis", "sum-mode");
    return mode.get("none");
}

} /* namespace borealis */

#include "Util/unmacros.h"
