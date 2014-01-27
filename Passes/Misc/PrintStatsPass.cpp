/*
 * PrintStatsPass.cpp
 *
 *  Created on: Jan 22, 2014
 *      Author: belyaev
 */

#include "PrintStatsPass.h"

#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Statistics/statisticsRegistry.h"
#include "Util/passes.hpp"
#include "Util/string_ref.hpp"

namespace borealis {

using namespace config;

namespace {

MultiConfigEntry StatsToShow{"statistics", "show"};

struct FilteredStats {
    StatisticsRegistry* reg;
    const util::string_ref* row;
    const util::string_ref* key;
};

std::ostream& operator<<(std::ostream& ost, const FilteredStats& fs) {
    fs.reg->print(ost, *fs.row, *fs.key);
    return ost;
}

} /* empty namespace */

bool PrintStatsPass::runOnModule(llvm::Module&) {
    auto& sr = StatisticsRegistry::instance();

    for(util::string_ref s : StatsToShow) {
        util::string_ref ls, rs;
        std::tie(ls, rs) = s.split('.');

        infos() << FilteredStats{&sr, &ls, &rs} << endl;
    }

    return false;
}

char PrintStatsPass::ID = 42;
static RegisterPass<PrintStatsPass>
X("stats", "Print statistics");

} /* namespace borealis */
