/*
 * DefectSummaryPass.cpp
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#include <clang/Basic/SourceManager.h>
#include <llvm/Support/CommandLine.h>

#include "Codegen/llvm.h"
#include "Passes/Checkers.def"
#include "Passes/DataProvider.hpp"
#include "Passes/DefectSummaryPass.h"
#include "Util/json.hpp"
#include "Util/locations.h"
#include "Util/passes.hpp"

namespace borealis {

static llvm::cl::opt<bool>
DumpOutput("dump-output", llvm::cl::init(false), llvm::cl::NotHidden,
  llvm::cl::desc("Dump analysis results to JSON"));

static llvm::cl::opt<std::string>
OutputFile("dump-output-file", llvm::cl::init(""), llvm::cl::NotHidden,
  llvm::cl::desc("JSON output file for analysis results"));

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

    auto& dm = GetAnalysis<DefectManager>::doit(this);
    auto& sm = GetAnalysis<DataProvider<clang::SourceManager>>::doit(this).provide();

    for (auto& defect : dm) {
        Locus origin = defect.location;
        Locus beg  { origin.filename, LocalLocus { origin.loc.line - 3, 1U } };
        Locus prev { origin.filename, LocalLocus { origin.loc.line    , 1U } };
        Locus next { origin.filename, LocalLocus { origin.loc.line + 1, 1U } };
        Locus end  { origin.filename, LocalLocus { origin.loc.line + 3, 1U } };
        LocusRange before { beg,  prev };
        LocusRange line   { prev, next };
        LocusRange after  { next, end  };

        llvm::StringRef ln = getRawSource(sm, line);
        std::string pt = ln.str();

        for (auto i = ln.find_first_not_of("\t\n "); i < ln.size(); ++i) {
            pt[i] = '~';
        }

        pt[origin.loc.col-1] = '^';

        infos() << DefectTypeNames.at(defect.type)
                << " at " << origin << endl
                << getRawSource(sm, before)
                << ln
                << pt << endl
                << getRawSource(sm, after) << endl;
    }

    infos() << util::jsonify(dm.getData()) << endl;

    return false;
}

void DefectSummaryPass::print(llvm::raw_ostream&, const llvm::Module*) const {}

char DefectSummaryPass::ID;
static RegisterPass<DefectSummaryPass>
X("defect-summary", "Pass that outputs located defects");

} /* namespace borealis */
