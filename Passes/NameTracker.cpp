/*
 * NameTracker.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: belyaev
 */

#include "NameTracker.h"

namespace borealis {

static void addName(llvm::Value& V, NameTracker::nameResolver_t& resolver) {
    if(V.hasName()) resolver[V.getName()] = &V;
}

bool NameTracker::runOnModule(llvm::Module& M) {
	using llvm::Module;
	using llvm::Function;
	using llvm::BasicBlock;
	using llvm::GlobalVariable;
	using llvm::Argument;
	using llvm::Instruction;

	for(auto& G : M.getGlobalList()) {
	    addName(G, globalResolver);
	}
	for(auto& F : M) {
	    addName(F, globalResolver);
		for(auto& A : F.getArgumentList()) {
		    addName(A, globalResolver);
		}
		for(auto& BB : F) {
			for(auto& I: BB) {
			    addName(I, globalResolver);
			}
		}
	}
	return false;
}

void NameTracker::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
	Info.setPreservesAll();
}

} // namespace borealis

char borealis::NameTracker::ID = 0;
static llvm::RegisterPass<borealis::NameTracker>
X("name_tracker", "Pass used to track value names in llvm IR", false, false);

