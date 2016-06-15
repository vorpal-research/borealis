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

void AbstractPredicateStateAnalysis::init() {
    initialState.reset();
    instructionStates.clear();
}

void AbstractPredicateStateAnalysis::finalize() {}

void AbstractPredicateStateAnalysis::printInstructionStates(llvm::raw_ostream&, const llvm::Module*) const {
    infos() << "Predicate state analysis results" << endl;
    infos() << "Initial state" << endl
            << initialState << endl;
    for (auto&& e : instructionStates) {
        infos() << *e.first << endl
                << e.second << endl;
    }
    infos() << "End of predicate state analysis results" << endl;
}

PredicateState::Ptr AbstractPredicateStateAnalysis::getInitialState() const {
    return initialState;
}

PredicateState::Ptr AbstractPredicateStateAnalysis::getInstructionState(const llvm::Instruction* I) const {
    if (borealis::util::containsKey(instructionStates, I)) {
        return instructionStates.at(I);
    } else {
        return nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////

PredicateStateAnalysis::PredicateStateAnalysis() :
        ProxyFunctionPass(ID), delegate(nullptr) {}
PredicateStateAnalysis::PredicateStateAnalysis(llvm::Pass* pass) :
        ProxyFunctionPass(ID, pass), delegate(nullptr) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    if ("one-for-one" == Mode()) {
        AUX<OneForOne>::addRequiredTransitive(AU);
    } else if ("one-for-all" == Mode()) {
        AUX<OneForAll>::addRequiredTransitive(AU);
    } else {
        BYE_BYE_VOID("Unknown PSA mode: " + Mode());
    }

    AUX<FunctionManager>::addRequiredTransitive(AU);
    AUX<LocationManager>::addRequiredTransitive(AU);
    AUX<NameTracker>::addRequiredTransitive(AU);
    AUX<SlotTrackerPass>::addRequiredTransitive(AU);
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    if ("one-for-one" == Mode()) {
        delegate = &GetAnalysis<OneForOne>::doit(this, F);
    } else if ("one-for-all" == Mode()) {
        delegate = &GetAnalysis<OneForAll>::doit(this, F);
    } else {
        BYE_BYE(bool, "Unknown PSA mode: " + Mode());
    }

    FN = FactoryNest(F.getDataLayout(), GetAnalysis<SlotTrackerPass>::doit(this, F).getSlotTracker(F));

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

    auto&& FM = GetAnalysis<FunctionManager>::doit(this);

    // No summary if:
    // - function does not return
    // - function returns void
    // - function returns non-pointer

    auto* RI = getSingleRetOpt(&F);
    // Function does not return, therefore has no useful summary
    if (not RI) return;

    auto&& initial = delegate->getInitialState();
    auto&& riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));

    auto&& bdy = riState->sliceOn(initial);
    ASSERT(bdy, "Function state slicing failed for: " + llvm::valueSummary(RI));

    FM.update(&F, bdy);
}

// Generate interpolation-based function summary
void PredicateStateAnalysis::updateInterpolSummary(llvm::Function& F) {

    using borealis::mathsat_::unlogic::undoThat;
    using borealis::util::view;

    USING_SMT_IMPL(MathSAT);

    auto&& FM = GetAnalysis<FunctionManager>::doit(this);
    auto&& NT = GetAnalysis<NameTracker>::doit(this);

    // No summary for main function
    if (llvm::isMain(F)) return;

    auto* RI = getSingleRetOpt(&F);
    if (not RI) return;

    std::vector<Term::Ptr> pointers;

    auto* retVal = RI->getReturnValue();
    if (nullptr != retVal and retVal->getType()->isPointerTy()) {
        pointers.push_back(FN.Term->getReturnValueTerm(&F));
    }

    std::vector<Term::Ptr> args;
    args.reserve(F.arg_size());
    for (auto&& arg : view(F.arg_begin(), F.arg_end())) {
        auto&& argTerm = FN.Term->getArgumentTerm(&arg);
        args.push_back(argTerm);
        if (arg.getType()->isPointerTy()) {
            pointers.push_back(argTerm);
        }
    }

    // No summary if:
    // - function returns non-pointer
    // - function has no pointer arguments
    if (pointers.empty()) return;

    auto&& initial = delegate->getInitialState();
    auto&& riState = delegate->getInstructionState(RI);
    ASSERT(riState, "No state found for: " + llvm::valueSummary(RI));

    auto&& bdy = riState->sliceOn(initial);
    ASSERT(bdy, "Function state slicing failed for: " + llvm::valueSummary(RI));

    auto&& PSB = FN.State * FN.State->Basic();
    for (auto&& ptr : pointers) {
        PSB += FN.Predicate->getEqualityPredicate(
                       FN.Term->getCmpTerm(
                           llvm::ConditionType::GT,
                           FN.Term->getBoundTerm(ptr),
                           FN.Term->getIntTerm(0, 64, llvm::Signedness::Unsigned) // XXX: 64???
                       ),
                       FN.Term->getTrueTerm()
               );
    }
    auto&& query = PSB();

    dbgs() << "Generating summary: " << endl
           << "  At: " << llvm::valueSummary(RI) << endl
           << "  Query: " << query << endl
           << "  State: " << bdy << endl;

    auto&& fMemInfo = FM.getMemoryBounds(&F);

    ExprFactory ef;
    Solver s(ef, fMemInfo.first, fMemInfo.second);

    auto&& itp = s.getSummary(args, query, bdy);

    auto&& summ = TermRebinder(F, &NT, FN).transform(undoThat(itp));

    auto&& summ_ = FN.State * FN.Predicate->getEqualityPredicate(summ, FN.Term->getTrueTerm());

    FM.update(&F, summ_());
}

void PredicateStateAnalysis::updateVisitedLocs(llvm::Function& F) {
    auto&& LM = GetAnalysis<LocationManager>::doit(this);

    auto* RI = getSingleRetOpt(&F);
    if (not RI) return;

    auto&& riState = delegate->getInstructionState(RI);
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
    static config::ConfigEntry<std::string> mode("summary", "sum-mode");
    return mode.get("none");
}

bool PredicateStateAnalysis::OptimizeStates() {
    static config::ConfigEntry<bool> optimizeStates("analysis", "optimize-states");
    return optimizeStates.get(false);
}

} /* namespace borealis */

#include "Util/unmacros.h"
