/*
 * DefectSummaryPass.cpp
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#include <clang/Basic/SourceManager.h>
#include <llvm/Support/CommandLine.h>

#include <fstream>

#include "Codegen/llvm.h"
#include "Passes/Checker/Checkers.def"
#include "Passes/DataProvider.hpp"
#include "Passes/DefectSummaryPass.h"
#include "Util/json.hpp"
#include "Util/passes.hpp"

namespace borealis {

typedef DataProvider<clang::SourceManager> DPSourceManager;

static llvm::cl::opt<bool>
DumpOutput("dump-output", llvm::cl::init(false), llvm::cl::NotHidden,
  llvm::cl::desc("Dump analysis results to JSON"));

static llvm::cl::opt<std::string>
DumpOutputFile("dump-output-file", llvm::cl::init(""), llvm::cl::NotHidden,
  llvm::cl::desc("JSON output file for analysis results"));

void DefectSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<DefectManager>::addRequiredTransitive(AU);
    AUX<DPSourceManager>::addRequiredTransitive(AU);

#define HANDLE_CHECKER(Checker) \
    AUX<Checker>::addRequiredTransitive(AU);
#include "Passes/Checker/Checkers.def"
}

bool DefectSummaryPass::runOnModule(llvm::Module&) {
    using clang::SourceManager;

    auto& dm = GetAnalysis<DefectManager>::doit(this);
    auto& sm = GetAnalysis<DPSourceManager>::doit(this).provide();

    for (auto& defect : dm) {
        Locus origin = defect.location;

        if (!origin) {
            infos() << DefectTypeNames.at(defect.type)
                    << " (cannot trace location)"
                    << endl;
            continue;
        }

        auto beg = origin.advanceLine(-3);
        auto prev = origin.advanceLine(0);
        auto next = origin.advanceLine(1);
        auto end = origin.advanceLine(3);

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

    if (DumpOutput || !DumpOutputFile.empty()) {
        if (DumpOutputFile.empty()) {
            DumpOutputFile = "borealis.json";
        }

        std::ofstream json(DumpOutputFile);
        json << util::jsonify(dm.getData());
        json.close();
    }

    return false;
}

char DefectSummaryPass::ID;
static RegisterPass<DefectSummaryPass>
X("defect-summary", "Pass that outputs located defects");

} /* namespace borealis */
