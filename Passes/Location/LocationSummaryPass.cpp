/*
 * LocationSummaryPass.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/CommandLine.h>
#include <yaml-cpp/yaml.h>

#include <fstream>

#include "Passes/Location/LocationManager.h"
#include "Passes/Location/LocationSummaryPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Passes/Util/DataProvider.hpp"

#include "Util/passes.hpp"

namespace borealis {

typedef DataProvider<clang::FileManager> DPFileManager;

static std::string DumpCoverageFileDefault = "%s.coverage";

static llvm::cl::opt<bool>
DumpCoverage("dump-coverage", llvm::cl::init(false), llvm::cl::NotHidden,
  llvm::cl::desc("Dump analysis coverage"));

static llvm::cl::opt<std::string>
DumpCoverageFile("dump-coverage-file", llvm::cl::init(DumpCoverageFileDefault), llvm::cl::NotHidden,
  llvm::cl::desc("Output file for analysis coverage"));

LocationSummaryPass::LocationSummaryPass(): ModulePass(ID) {}

void LocationSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
    AUX<LocationManager>::addRequiredTransitive(AU);
    AUX<DPFileManager>::addRequiredTransitive(AU);
}

bool LocationSummaryPass::runOnModule(llvm::Module& M) {
    auto& slt = GetAnalysis<SourceLocationTracker>::doit(this);
    auto& lm = GetAnalysis<LocationManager>::doit(this);

    // XXX: Maybe we'll need this in the future...
    // auto& fm = GetAnalysis<DPFileManager>::doit(this).provide();


    auto* mainFileEntry = M.getModuleIdentifier().c_str();

    if (DumpCoverage || DumpCoverageFile != DumpCoverageFileDefault) {

        util::replace("%s", mainFileEntry, DumpCoverageFile);

        auto& llvmLocs = lm.getLocations();

        std::map<std::string, std::list<Locus>> locMap;

        auto& normalLocs = locMap["normal"];
        for (const auto& v : llvmLocs) {
            auto loc = slt.getLocFor(v);
            if (loc.isUnknown()) continue;
            normalLocs.push_back(loc);
        }

        locMap["deadCode"] = locMap["chop"] = locMap["killed"];

        YAML::Emitter yaml;
        yaml.SetSeqFormat(YAML::Block);

        yaml << YAML::BeginDoc;
        yaml << locMap;
        yaml << YAML::EndDoc;

        std::ofstream output(DumpCoverageFile);
        output << yaml.c_str();
        output.close();
    }

    return false;
}

YAML::Emitter& operator<<(YAML::Emitter& yaml, const Locus& loc) {
    std::ostringstream oss;
    oss << loc;
    return yaml << YAML::LocalTag("SCL") << YAML::SingleQuoted << oss.str();
}

char LocationSummaryPass::ID;
static RegisterPass<LocationSummaryPass>
X("location-summary", "Pass that outputs visited locations");

} /* namespace borealis */
