/*
 * statistics.cpp
 *
 *  Created on: Jan 22, 2014
 *      Author: belyaev
 */

#include "Statistics/statistics.h"
#include "Statistics/statisticsRegistry.h"

#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>

namespace borealis {

struct StatisticImplBase {
    unsigned value;
    const std::string row;
    const std::string key;
    const std::string desc;

    StatisticImplBase(
        unsigned value,
        const std::string& row,
        const std::string& key,
        const std::string& desc
    ):
        value(value), row(row), key(key), desc(desc){}
};

struct Statistic::Impl: StatisticImplBase {
    Impl(
        unsigned value,
        const std::string& row,
        const std::string& key,
        const std::string& desc
    ): StatisticImplBase(value, row, key, desc){};
};

void Statistic::advance(long delta) {
    if(delta < 0) pImpl->value -= static_cast<unsigned>(-delta);
    else pImpl->value += static_cast<unsigned>(delta);
}

unsigned Statistic::get() const{
    return pImpl->value;
}

struct StatisticsRegistry::Impl {
    typedef std::unordered_map<std::string, std::shared_ptr<StatisticImplBase>> row;
    typedef std::unordered_map<std::string, row> partition;
    partition data;
};

StatisticsRegistry::StatisticsRegistry(): pImpl{new Impl{}} {};
StatisticsRegistry::~StatisticsRegistry() {};

StatisticsRegistry& StatisticsRegistry::instance() {
    static StatisticsRegistry instance_;
    return instance_;
}

Statistic::Statistic(const std::string& row, const std::string& key, const std::string& description) {
    auto& registry = StatisticsRegistry::instance();
    auto& curRow = registry.pImpl->data[row];

    auto resIt = curRow.find(key);
    if(resIt == std::end(curRow)) {
        curRow[key] = pImpl = std::make_shared<Impl>(0U, row, key, description);
    } else {
        pImpl = std::static_pointer_cast<Impl>(resIt->second);
    }
}

Statistic::~Statistic() {}

std::ostream& operator<<(std::ostream& ost, const StatisticsRegistry& reg) {
    for(auto& pr1 : reg.pImpl->data)
        for(auto& pr2 : pr1.second)
            ost << pr2.second->desc << ": " << pr2.second->value << "\n";
    return ost;
}


} /* namespace borealis */
