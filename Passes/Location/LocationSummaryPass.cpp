/*
 * LocationSummaryPass.cpp
 *
 *  Created on: Aug 7, 2013
 *      Author: snowball
 */

#include "Passes/Location/LocationManager.h"
#include "Passes/Location/LocationSummaryPass.h"
#include "Passes/Tracker/SourceLocationTracker.h"

#include "Util/passes.hpp"

namespace borealis {

LocationSummaryPass::LocationSummaryPass(): ModulePass(ID) {}

void LocationSummaryPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();
    AUX<SourceLocationTracker>::addRequiredTransitive(AU);
    AUX<LocationManager>::addRequiredTransitive(AU);
}

bool LocationSummaryPass::runOnModule(llvm::Module&) {
    auto& slt = GetAnalysis<SourceLocationTracker>::doit(this);
    auto& lm = GetAnalysis<LocationManager>::doit(this);

    infos() << "Visited locations:" << endl;
    for (const auto& value : lm.getLocations()) {
        infos() << slt.getLocFor(value) << endl;
    }
    return false;
}

char LocationSummaryPass::ID;
static RegisterPass<LocationSummaryPass>
X("location-summary", "Pass that outputs visited locations");

} /* namespace borealis */
