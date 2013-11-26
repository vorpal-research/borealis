/*
 * DefectSummaryPass.cpp
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/CommandLine.h>

#include <fstream>

#include "Codegen/llvm.h"
#include "Passes/Checker/Defines.def"
#include "Passes/Defect/DefectSummaryPass.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/json.hpp"
#include "Util/passes.hpp"
#include "Util/xml.hpp"

namespace borealis {

static std::string DumpOutputDefault = "";
static std::string DumpOutputFileDefault = "%s.report";

static llvm::cl::opt<std::string>
DumpOutput("dump-output", llvm::cl::init(DumpOutputDefault), llvm::cl::NotHidden,
  llvm::cl::desc("Dump analysis results to JSON/XML"));

static llvm::cl::opt<std::string>
DumpOutputFile("dump-output-file", llvm::cl::init(DumpOutputFileDefault), llvm::cl::NotHidden,
  llvm::cl::desc("Output file for analysis results"));

void DefectSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX<DefectManager>::addRequiredTransitive(AU);

#define HANDLE_CHECKER(Checker) \
    AUX<Checker>::addRequiredTransitive(AU);
#include "Passes/Checker/Defines.def"
}

bool DefectSummaryPass::runOnModule(llvm::Module& M) {

    auto& dm = GetAnalysis<DefectManager>::doit(this);

    auto* mainFileEntry = M.getModuleIdentifier().c_str();

    for (auto& defect : dm) {
        Locus origin = defect.location;

        if (!origin) {
            infos() << defect.type
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

        auto ln = getRawSource(line);
        auto pt = ln;

        for (auto i = ln.find_first_not_of("\t\n "); i < ln.size(); ++i) {
            pt[i] = '~';
        }

        pt[origin.loc.col-1] = '^';

        infos() << defect.type
                << " at " << origin << endl
                << getRawSource(before)
                << ln
                << pt << endl
                << getRawSource(after) << endl
                ;
    }

    if (DumpOutput != DumpOutputDefault || DumpOutputFile != DumpOutputFileDefault) {

        util::replace("%s", mainFileEntry, DumpOutputFile);

        if ("json" == DumpOutput) {
            std::ofstream json(DumpOutputFile);
            json << util::jsonify(dm.getData());
            json.close();
        } else if ("xml" == DumpOutput) {
            std::ofstream xml(DumpOutputFile);
            xml << (
                util::Xml("s2a-report")
                    >> "defects"
                        << util::Xml::ListOf("defect", dm.getData())
            );
            xml.close();
        }
    }

    return false;
}

char DefectSummaryPass::ID;
static RegisterPass<DefectSummaryPass>
X("defect-summary", "Pass that outputs located defects");

} /* namespace borealis */
