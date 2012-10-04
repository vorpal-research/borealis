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

namespace borealis {

using util::contains;
using util::containsKey;
using util::for_each;
using util::streams::endl;
using util::toString;

typedef PredicateAnalysis::PredicateMap PM;
typedef PredicateAnalysis::TerminatorPredicateMap TPM;
typedef PredicateStateAnalysis::PredicateState PS;
typedef PredicateStateAnalysis::PredicateStateVector PSV;

PSV createPSV() {
	PSV res = PSV();
	res.push_back(PS());
	return res;
}

PSV addToPSV(const PSV& to, const Predicate* pred) {
	using namespace::std;

	PSV res = to;
	for_each(res, [&pred](PS& state){
		state.insert(pred);
	});
	return res;
}

std::pair<PSV, bool> mergePSV(const PSV& to, const PSV& from) {
	using namespace::std;
	using namespace::llvm;

	PSV res = PSV();
	bool changed = false;

	for (auto t = to.begin(); t != to.end(); ++t)
	{
		const PS& TO = *t;
		res.push_back(TO);
	}

	for (auto f = from.begin(); f != from.end(); ++f)
	{
		const PS& FROM = *f;
		if (contains(res, FROM)) {
			// DO NOTHING
		} else {
			res.push_back(FROM);
			changed = true;
		}
	}

	return make_pair(res, changed);
}

PredicateStateAnalysis::PredicateStateAnalysis() : llvm::FunctionPass(ID) {
	// TODO
}

void PredicateStateAnalysis::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<PredicateAnalysis>();
}

bool PredicateStateAnalysis::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;

	PA = &getAnalysis<PredicateAnalysis>();

	workQueue.push(make_pair(&F.getEntryBlock(), createPSV()));
	processQueue();

	errs() << endl << "PSA results:" << endl;
	for_each(predicateStateMap, [this](const PredicateStateMapEntry& entry) {
		errs() << *entry.first << endl;
		errs() << entry.second << endl;
	});
	errs() << endl << "End of PSA results" << endl;

	return false;
}

void PredicateStateAnalysis::processQueue() {
	while (!workQueue.empty()) {
		processBasicBlock(workQueue.front());
		workQueue.pop();
	}
}

void PredicateStateAnalysis::processBasicBlock(const WorkQueueEntry& wqe) {
	using namespace::std;
	using namespace::llvm;

	PM& pm = PA->getPredicateMap();

	const BasicBlock& bb = *(wqe.first);
	PSV inStateVec = wqe.second;
	bool changed = false;

	for (auto inst = bb.begin(); inst != bb.end(); ++inst) {
		const Instruction& I = *inst;
		PSV stateVec;

		bool hasPredicate = containsKey(pm, &I);
		bool hasState = containsKey(predicateStateMap, &I);

		if (hasPredicate) {
			if (hasState) {
				stateVec = predicateStateMap[&I];
			} else {
				stateVec = PSV();
				changed = true;
			}
		} else {
			continue;
		}

		PSV modifiedInStateVec = addToPSV(inStateVec, pm[&I]);
		pair<PSV, bool> merged = mergePSV(stateVec, modifiedInStateVec);

		predicateStateMap[&I] = merged.first;
		changed = changed || merged.second;

		if (!changed) {
			break;
		}

		inStateVec = merged.first;
	}

	if (changed) {
		processTerminator(*bb.getTerminator(), inStateVec);
	}
}

void PredicateStateAnalysis::processTerminator(
		const llvm::TerminatorInst& I,
		const PSV& state) {
	using namespace::llvm;

	if (isa<BranchInst>(I))
		{ process(cast<BranchInst>(I), state); }
}

void PredicateStateAnalysis::process(
		const llvm::BranchInst& I,
		const PSV& state) {
	using namespace::std;
	using namespace::llvm;

	TPM& tpm = PA->getTerminatorPredicateMap();

	if (I.isUnconditional()) {
		const BasicBlock* succ = I.getSuccessor(0);
		workQueue.push(make_pair(succ, state));
	} else {
		const BasicBlock* trueSucc = I.getSuccessor(0);
		const BasicBlock* falseSucc = I.getSuccessor(1);

		const Predicate* truePred = tpm[make_pair(&I, trueSucc)];
		const Predicate* falsePred = tpm[make_pair(&I, falseSucc)];

		workQueue.push(make_pair(trueSucc, addToPSV(state, truePred)));
		workQueue.push(make_pair(falseSucc, addToPSV(state, falsePred)));
	}
}

PredicateStateAnalysis::~PredicateStateAnalysis() {
	// TODO
}

} /* namespace borealis */

char borealis::PredicateStateAnalysis::ID;
static llvm::RegisterPass<borealis::PredicateStateAnalysis>
X("predicate-state", "Predicate state analysis", false, false);
