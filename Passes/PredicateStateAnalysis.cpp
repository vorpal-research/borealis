/*
 * PredicateStateAnalysis.cpp
 *
 *  Created on: Sep 4, 2012
 *      Author: ice-phoenix
 */

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include "PredicateStateAnalysis.h"

namespace borealis {

using util::contains;
using util::containsKey;
using util::for_each;
using util::streams::endl;
using util::toString;

typedef PredicateAnalysis::PredicateMap PM;
typedef PredicateStateAnalysis::PredicateState PS;
typedef PredicateStateAnalysis::PredicateStateVector PSV;

PSV createPSV() {
	return PSV();
}

PSV addToPSV(const PSV& to, std::string& pred) {
	using namespace::std;

	PSV res = createPSV();
	for_each(to, [&pred, &res](const PS& state){
		PS ps = PS(state.begin(), state.end());
		ps.insert(&pred);
		res.push_back(ps);
	});
	return res;
}

std::pair<PSV, bool> mergePSV(const PSV& to, const PSV& from) {
	using namespace::std;
	using namespace::llvm;

	PSV res = createPSV();
	bool changed = false;

	if (from.empty()) {
		for_each(to, [&res](const PS& state) {
			res.push_back(PS(state.begin(), state.end()));
		});
		return make_pair(res, changed);
	}

	for (auto t = to.begin(); t != to.end(); ++t)
	{
		const PS& TO = *t;

		for (auto f = from.begin(); f != from.end(); ++f)
		{
			const PS& FROM = *f;

			auto R = vector<string*>(TO.size() + FROM.size());
			merge(TO.begin(), TO.end(), FROM.begin(), FROM.end(), R.begin());

			PS S = PS(R.begin(), R.end());
			res.push_back(S);

			changed = changed || S.size() > TO.size();
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

	PA = nullptr;

	errs() << "\nPSA results:\n";
	for_each(predicateStateMap, [this](const PredicateStateMapEntry& entry) {
		errs() << *entry.first << "\n";
		errs() << entry.second << "\n";
	});
	errs() << "\nEnd of PSA results:\n";

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
				stateVec = createPSV();
				PS state = PredicateState();
				state.insert(&pm[&I]);
				stateVec.push_back(state);
				changed = true;
			}
		} else {
			continue;
		}

		pair<PSV, bool> merged = mergePSV(stateVec, inStateVec);

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
	using namespace::llvm;

	PM& pm = PA->getPredicateMap();

	if (I.isUnconditional()) {
		BasicBlock* succ = I.getSuccessor(0);
		workQueue.push(make_pair(succ, state));
	} else {
		const Value* cond = I.getCondition();

		if (isa<Instruction>(*cond)) {
			const Instruction& condi = cast<Instruction>(*cond);
			if (containsKey(pm, &condi)) {
				std::string& pred = pm[&condi];

				std::string& truePred = pred;
				BasicBlock* trueSucc = I.getSuccessor(0);
				workQueue.push(make_pair(trueSucc, addToPSV(state, truePred)));

				std::string& falsePred = pred;
				BasicBlock* falseSucc = I.getSuccessor(1);
				workQueue.push(make_pair(falseSucc, addToPSV(state, falsePred)));
			} else {
				errs() << "Terminator does not introduce any predicate information: "
						<< I
						<< endl;
				BasicBlock* trueSucc = I.getSuccessor(0);
				workQueue.push(make_pair(trueSucc, state));
				BasicBlock* falseSucc = I.getSuccessor(1);
				workQueue.push(make_pair(falseSucc, state));
			}
		} else {
			errs() << "Terminator depends on non-instruction value: "
									<< I
									<< endl
									<< *cond
									<< endl;
		}
	}
}

PredicateStateAnalysis::~PredicateStateAnalysis() {
	// TODO
}

} /* namespace borealis */

char borealis::PredicateStateAnalysis::ID = 20;
static llvm::RegisterPass<borealis::PredicateStateAnalysis>
X("predicate-state", "Predicate state analysis", false, false);
