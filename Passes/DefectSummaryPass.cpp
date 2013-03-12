/*
 * DefectSummaryPass.cpp
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#include "Passes/DefectSummaryPass.h"
#include "Util/passes.hpp"

#include "Passes/Checkers.def"

namespace borealis {

void DefectSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<DefectManager>::addRequiredTransitive(AU);

#define HANDLE_CHECKER(Checker) \
    AUX<Checker>::addRequiredTransitive(AU);
#include "Passes/Checkers.def"

}

bool DefectSummaryPass::runOnModule(llvm::Module&) {
    return false;
}

void DefectSummaryPass::print(llvm::raw_ostream& O, const llvm::Module* M) const {
    GetAnalysis<DefectManager>::doit(this).print(O, M);
}

char DefectSummaryPass::ID;
static RegisterPass<DefectSummaryPass>
X("defect-summary", "Pass that outputs located defects");

} /* namespace borealis */
