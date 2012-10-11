/*
 * NameTracker.cpp
 *
 *  Created on: Oct 11, 2012
 *      Author: belyaev
 */

#include "NameTracker.h"

namespace borealis {

bool NameTracker::runOnModule(llvm::Module& M) {
	using llvm::Module;
	using llvm::Function;
	using llvm::BasicBlock;
	using llvm::GlobalVariable;
	using llvm::Argument;
	using llvm::Instruction;

	for(auto& G : M.getGlobalList()) {
		if(G.hasName()) globalResolver[G.getName()] = &G;
	}
	for(auto& F : M) {
		if(F.hasName()) globalResolver[F.getName()] = &F;
		for(auto& A: F.getArgumentList()) {
			if(A.hasName()) localResolvers[&F][A.getName()] = &A;
		}
		for(auto& BB: F) {
			for(auto& I: BB) {
				if(I.hasName()) localResolvers[&F][I.getName()] = &I;
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

