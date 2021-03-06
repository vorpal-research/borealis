/*
 * location_container.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef SOURCE_LOCATION_TRACKER_LOCATION_CONTAINER_H
#define SOURCE_LOCATION_TRACKER_LOCATION_CONTAINER_H

#include <map>
#include <unordered_map>

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

template<class T>
class location_container {

public:

    typedef std::multimap< Locus, T > l2t_t;
    typedef std::unordered_map< T, Locus > t2l_t;

    typedef typename l2t_t::iterator iterator;
    typedef std::pair<iterator, iterator> range;

    typedef typename l2t_t::const_iterator const_iterator;
    typedef std::pair<const_iterator, const_iterator> const_range;

private:

    l2t_t l2t;
    t2l_t t2l;

public:

    void put(const Locus& loc, T val) {
        using borealis::util::removeFromMultimap;

        if (t2l.count(val)) {
            removeFromMultimap(l2t, loc, val);
        }
        t2l[val] = loc;
        l2t.insert({loc, val});
    }

    bool contains(const Locus& loc) const {
        return l2t.count(loc) > 0;
    }


    bool contains(const T& val) const {
        return t2l.count(val) > 0;
    }

    template<class U, class = GUARD(util::pointers_to_same<U, T>::value)>
    bool contains(U val) const {
        return t2l.count(const_cast<T>(val)) > 0;
    }

    const Locus& operator[](const T& key) const {
        return t2l.at(key);
    }

    template<class U, class = GUARD(util::pointers_to_same<U, T>::value)>
    const Locus& operator[](U key) const {
        return t2l.at(const_cast<T>(key));
    }

    const_range range_after(const Locus& loc) const {
        auto start = l2t.upper_bound(loc);
        auto end = start;
        while (end != l2t.end() && end->first == start->first) ++end;
        return std::make_pair(start, end);
    }

    const l2t_t& getFrom() const { return l2t; }
    const t2l_t& getTo() const { return t2l; }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif // SOURCE_LOCATION_TRACKER_LOCATION_CONTAINER_H
