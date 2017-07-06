/*
 * DefectSummaryPass.cpp
 *
 *  Created on: Mar 12, 2013
 *      Author: belyaev
 */

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>

#include <fstream>

#include "Codegen/llvm.h"
#include "Config/config.h"
#include "Passes/Checker/Defines.def"
#include "Passes/Defect/DefectSummaryPass.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/json.hpp"
#include "Util/passes.hpp"
#include "Util/xml.hpp"

namespace borealis {

const std::string DumpOutputDefault = "";
const std::string DumpOutputFileDefault = "%s.report";

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

    std::vector<DefectInfo> results(dm.begin(), dm.end());
    std::sort(results.begin(), results.end(), [](DefectInfo& a, DefectInfo& b) -> bool {
        return a < b;
    });
    for (auto& defect : results) {
        Locus origin = defect.location;

        if (!origin) {
            infos() << defect.type
                    << " (cannot trace location)"
                    << endl
                    << (dm.getAdditionalInfo(defect).runResult == AdditionalDefectInfo::RunResult::Disproven ? "Disproven by tassadar\n" : "");
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

        if(origin.loc.col) pt[origin.loc.col-1] = '^';

        infos() << defect.type
                << " at " << origin << endl
                << getRawSource(before)
                << ln
                << pt << endl
                << getRawSource(after) << endl
                << (dm.getAdditionalInfo(defect).runResult == AdditionalDefectInfo::RunResult::Disproven ? "Disproven by tassadar\n" : "")
                ;
    }

    static config::StringConfigEntry DumpOutputOpt("output", "dump-output");
    static config::StringConfigEntry DumpOutputFileOpt("output", "dump-output-file");

    auto DumpOutput = DumpOutputOpt.get(DumpOutputDefault);
    auto DumpOutputFile = DumpOutputFileOpt.get(DumpOutputFileDefault);

    if (DumpOutput != DumpOutputDefault ||
        DumpOutputFile != DumpOutputFileDefault) {

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
