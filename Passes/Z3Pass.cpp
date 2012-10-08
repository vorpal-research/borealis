/*
 * Z3Pass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: ice-phoenix
 */

#include "Z3Pass.h"

#include <z3/z3++.h>

namespace borealis {

Z3Pass::Z3Pass() : llvm::FunctionPass(ID) {
	// TODO
}

void Z3Pass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	using namespace::llvm;

	Info.setPreservesAll();
}

bool Z3Pass::runOnFunction(llvm::Function& F) {
	using namespace::std;
	using namespace::llvm;
	using namespace::z3;

    // TODO: Implement

	return false;
}

Z3Pass::~Z3Pass() {
	// TODO
}

} /* namespace borealis */

char borealis::Z3Pass::ID;
static llvm::RegisterPass<borealis::Z3Pass>
X("z3", "Predicate SMT analysis via Z3", false, false);
