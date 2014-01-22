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

namespace borealis {

using namespace config;

namespace {

MultiConfigEntry StatsToShow{"statistics", "show"};

struct FilteredStats {
    StatisticsRegistry* reg;
    const std::string* row;
    const std::string* key;
};

std::ostream& operator<<(std::ostream& ost, const FilteredStats& fs) {
    fs.reg->print(ost, *fs.row, *fs.key);
    return ost;
}

} /* empty namespace */

bool PrintStatsPass::runOnModule(llvm::Module&) {
    auto& sr = StatisticsRegistry::instance();

    for(auto& s : StatsToShow) {
        auto dot = s.find_first_of('.');
        if(dot != std::string::npos) {
            auto ls = s.substr(0, dot);
            auto rs = s.substr(std::min(s.size()-1, dot+1));

            infos() << FilteredStats{&sr, &ls, &rs} << endl;
        }
    }

    return false;
}

char PrintStatsPass::ID = 42;
static RegisterPass<PrintStatsPass>
X("stats", "Print statistics");

} /* namespace borealis */
