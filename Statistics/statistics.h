/*
 * statistics.h
 *
 *  Created on: Jan 21, 2014
 *      Author: belyaev
 */

#ifndef STATISTICS_H_
#define STATISTICS_H_

#include <memory>

namespace borealis {

class Statistic {
    struct Impl; std::shared_ptr<Impl> pImpl;

    unsigned get() const;
    void advance(long delta = 1);

public:
    Statistic(const std::string& row, const std::string& key, const std::string& decription);
    ~Statistic();

    operator unsigned () const { return get(); }

    Statistic& operator++() { advance(); return *this; }
    unsigned operator++(int) { unsigned tmp = *this; advance(); return tmp; }
    Statistic& operator--() { advance(-1); return *this; }
    unsigned operator--(int) { unsigned tmp = *this; advance(-1); return tmp; }
    Statistic& operator+=(unsigned delta) { advance(static_cast<long>(delta)); return *this; }
    Statistic& operator-=(unsigned delta) { advance(-static_cast<long>(delta)); return *this; }
};

} /* namespace borealis */

#endif /* STATISTICS_H_ */
