/*
 * LocationSummaryPass.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#include "LocationSummaryPass.h"
#include "Passes/Location/LocationManager.h"
#include "Passes/Tracker/SourceLocationTracker.h"
#include "Util/passes.hpp"

namespace borealis {

LocationSummaryPass::LocationSummaryPass(): ModulePass(ID) {}

void LocationSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<LocationManager>::addRequiredTransitive(AU);
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
}

bool LocationSummaryPass::runOnModule(llvm::Module&) {
    auto& slt = GetAnalysis<SourceLocationTracker>::doit(this);
    auto& lm = GetAnalysis<LocationManager>::doit(this);

    infos() << "Visited locations:\n";
    for (const auto value : lm.getLocations()) {
        infos() << slt.getFilenameFor(value) << ' '
                << slt.getLineFor(value) << ' '
                << slt.getColumnFor(value) << '\n';
    }
    return false;
}

char LocationSummaryPass::ID;
static RegisterPass<LocationSummaryPass>
X("location-summary", "Pass that outputs locations visited by other passes");

} /* namespace borealis */

