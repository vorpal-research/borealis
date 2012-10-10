/*
 * Z3Pass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: ice-phoenix
 */

#include "Z3Pass.h"

#include <z3/z3++.h>

#include "PredicateStateAnalysis.h"

namespace borealis {

using util::streams::endl;

typedef PredicateState PS;
typedef PredicateStateAnalysis::PredicateStateMapEntry PSME;

Z3Pass::Z3Pass() : llvm::FunctionPass(ID) {
	// TODO
}

void Z3Pass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
	Info.addRequiredTransitive<PredicateStateAnalysis>();
}

bool Z3Pass::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;
	using namespace::z3;

    auto PSA = &getAnalysis<PredicateStateAnalysis>();

    for_each(PSA->getPredicateStateMap(), [this](const PSME& psme) {
    	auto psv = psme.second;
    	for_each(psv, [this](const PS& ps){
    	    context ctx;
    		solver s(ctx);

    		auto z3 = ps.toZ3(ctx);
			s.add(z3);

    		errs() << ps << endl;
    		errs() << s.check() << endl;
    	});
    });

	return false;
}

Z3Pass::~Z3Pass() {
	// TODO
}

} /* namespace borealis */

char borealis::Z3Pass::ID;
static llvm::RegisterPass<borealis::Z3Pass>
X("z3", "Predicate SMT analysis via Z3", false, false);
