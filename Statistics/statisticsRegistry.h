/*
 * StatisticsRegistry.h
 *
 *  Created on: Jan 21, 2014
 *      Author: ice-phoenix
 */

#ifndef STATISTICS_REGISTRY_H
#define STATISTICS_REGISTRY_H

#include <memory>
#include <iosfwd>

#include "Statistics/statistics.h"

namespace borealis {

class StatisticsRegistry {
    struct Impl; std::unique_ptr<Impl> pImpl;

    StatisticsRegistry();
public:
    friend class Statistic;

    ~StatisticsRegistry();

    static StatisticsRegistry& instance();

    friend std::ostream& operator<<(std::ostream&, const StatisticsRegistry&);

    void print(std::ostream&, const std::string&, const std::string&) const;
};

} /* namespace borealis */

#endif /* STATISTICS_REGISTRY_H */
