/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#include "PredicateStateAnalysis.h"

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Support/raw_ostream.h>

#include "Solver/Z3ExprFactory.h"
#include "Solver/Z3Solver.h"

namespace borealis {

typedef PredicateAnalysis::PredicateMap PM;
typedef PredicateAnalysis::TerminatorPredicateMap TPM;

PredicateStateAnalysis::PredicateStateAnalysis() : llvm::FunctionPass(ID) {}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const{
    using namespace::llvm;

    Info.setPreservesAll();
    Info.addRequiredTransitive<PredicateAnalysis>();
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
    init();

    PA = &getAnalysis<PredicateAnalysis>();

    workQueue.push(std::make_pair(&F.getEntryBlock(), PredicateStateVector(true)));
    processQueue();

    removeUnreachableStates();

    return false;
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

    PM& pm = PA->getPredicateMap();

    const BasicBlock& bb = *(wqe.first);
    PredicateStateVector inStateVec = wqe.second;
    bool shouldScheduleTerminator = true;

    for (auto& I : bb) {
        PredicateStateVector stateVec;

        bool hasPredicate = containsKey(pm, &I);
        bool hasState = containsKey(predicateStateMap, &I);

        if (!hasPredicate) continue;
        if (hasState) stateVec = predicateStateMap[&I];

        PredicateStateVector modifiedInStateVec =
                inStateVec.addPredicate(pm[&I]);
        PredicateStateVector merged =
                stateVec.merge(modifiedInStateVec);

        if (stateVec == merged) {
            shouldScheduleTerminator = false;
            break;
        }

        predicateStateMap[&I] = inStateVec = merged;
    }

    if (shouldScheduleTerminator) {
        processTerminator(*bb.getTerminator(), inStateVec);
    }
}

void PredicateStateAnalysis::processTerminator(
        const llvm::TerminatorInst& I,
        const PredicateStateVector& state) {
    using namespace::llvm;

    if (isa<BranchInst>(I))
    { process(cast<BranchInst>(I), state); }
}

void PredicateStateAnalysis::process(
        const llvm::BranchInst& I,
        const PredicateStateVector& state) {
    using namespace::llvm;

    TPM& tpm = PA->getTerminatorPredicateMap();

    if (I.isUnconditional()) {
        const BasicBlock* succ = I.getSuccessor(0);
        workQueue.push(std::make_pair(succ, state));
    } else {
        const BasicBlock* trueSucc = I.getSuccessor(0);
        const BasicBlock* falseSucc = I.getSuccessor(1);

        const Predicate* truePred = tpm[std::make_pair(&I, trueSucc)];
        const Predicate* falsePred = tpm[std::make_pair(&I, falseSucc)];

        workQueue.push(std::make_pair(trueSucc, state.addPredicate(truePred)));
        workQueue.push(std::make_pair(falseSucc, state.addPredicate(falsePred)));
    }
}

bool isUnreachable(const PredicateState& ps) {
    using namespace::z3;

    context ctx;
    Z3ExprFactory z3ef(ctx);
    Z3Solver s(z3ef);

    return !s.checkPathPredicates(ps);
}

void PredicateStateAnalysis::removeUnreachableStates() {
    for (auto& psme : predicateStateMap){
        auto I = psme.first;
        PredicateStateVector psv = psme.second;
        predicateStateMap[I] = psv.remove_if(isUnreachable);
    }
}

PredicateStateAnalysis::~PredicateStateAnalysis() {}

char PredicateStateAnalysis::ID;
static llvm::RegisterPass<PredicateStateAnalysis>
X("predicate-state", "Predicate state analysis");

} /* namespace borealis */
