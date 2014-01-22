/*
 * PrintStatsPass.cpp
 *
 *  Created on: Jan 22, 2014
 *      Author: belyaev
 */

#include "PrintStatsPass.h"

#include "Logging/logger.hpp"
#include "Statistics/statisticsRegistry.h"
#include "Util/passes.hpp"

namespace borealis {

bool PrintStatsPass::runOnModule(llvm::Module&) {
    infos() << StatisticsRegistry::instance() << endl;
    return false;
}

char PrintStatsPass::ID = 42;
static RegisterPass<PrintStatsPass>
X("stats", "Print statistics");

} /* namespace borealis */
