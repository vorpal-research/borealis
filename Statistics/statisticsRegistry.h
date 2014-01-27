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
#include "Util/string_ref.hpp"

namespace borealis {

class StatisticsRegistry {
    struct Impl; std::unique_ptr<Impl> pImpl;

    StatisticsRegistry();
public:
    friend class Statistic;

    ~StatisticsRegistry();

    static StatisticsRegistry& instance();

    friend std::ostream& operator<<(std::ostream&, const StatisticsRegistry&);

    void print(std::ostream&, util::string_ref, util::string_ref) const;
};

} /* namespace borealis */

#endif /* STATISTICS_REGISTRY_H */
