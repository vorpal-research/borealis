/*
 * DefectSummaryPass.cpp
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#include <clang/Basic/SourceManager.h>

#include "Codegen/llvm.h"
#include "Passes/Checkers.def"
#include "Passes/DataProvider.hpp"
#include "Passes/DefectSummaryPass.h"
#include "Util/locations.h"
#include "Util/passes.hpp"

namespace borealis {

void DefectSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<DataProvider<clang::SourceManager>>::addRequiredTransitive(AU);

#define HANDLE_CHECKER(Checker) \
    AUX<Checker>::addRequiredTransitive(AU);
#include "Passes/Checkers.def"

}

bool DefectSummaryPass::runOnModule(llvm::Module&) {
    using clang::SourceManager;

    DefectManager& dm = GetAnalysis<DefectManager>::doit(this);
    const SourceManager& sm = GetAnalysis<DataProvider<clang::SourceManager>>::doit(this).provide();

    for (auto& defect : dm) {
        Locus origin = defect.second;
        Locus beg { origin.filename, LocalLocus { origin.loc.line - 3, 1U } };
        Locus end { origin.filename, LocalLocus { origin.loc.line + 3, 1U } };
        LocusRange range {beg, end};

        infos() << DefectTypeNames.at(defect.first) << endl;
        infos() << origin << endl;
        infos() << getRawSource(sm, range) << endl;
    }
    return false;
}

void DefectSummaryPass::print(llvm::raw_ostream&, const llvm::Module*) const {}

char DefectSummaryPass::ID;
static RegisterPass<DefectSummaryPass>
X("defect-summary", "Pass that outputs located defects");

} /* namespace borealis */
