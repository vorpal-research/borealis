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
#include <iomanip>

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

using row = std::unordered_map<std::string, std::shared_ptr<StatisticImplBase>>;
using partition = std::unordered_map<std::string, row>;

struct StatisticsRegistry::Impl {
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

static void printSingle(std::ostream& ost, const StatisticImplBase& impl) {
    ost.width(6);
    ost << std::right << impl.value << ": " << impl.desc << "\n";
}

static void printAll(std::ostream& ost, const partition& sr) {
    for(auto& pr1 : sr)
        for(auto& pr2 : pr1.second)
            printSingle(ost, *pr2.second);
}

static void printRow(std::ostream& ost, const row& sr) {
    for(auto& pr1 : sr) printSingle(ost, *pr1.second);
}

void StatisticsRegistry::print(std::ostream& ost,
    const std::string& row, const std::string& key) const{
    if(row == "*") {
        return printAll(ost, pImpl->data);
    }
    auto it = pImpl->data.find(row);
    if(it != std::end(pImpl->data)) {
        if(key == "*") {
            return printRow(ost, it->second);
        }

        auto it2 = it->second.find(key);
        if(it2 != std::end(it->second)) {
            return printSingle(ost, *it2->second);
        }
    }
}

std::ostream& operator<<(std::ostream& ost, const StatisticsRegistry& reg) {
    printAll(ost, reg.pImpl->data);
    return ost;
}


} /* namespace borealis */
