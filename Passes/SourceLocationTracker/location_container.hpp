
#ifndef SOURCE_LOCATION_TRACKER_LOCATION_CONTAINER_H
#define SOURCE_LOCATION_TRACKER_LOCATION_CONTAINER_H


#include <map>
#include <unordered_map>
#include <functional>
#include <utility>
#include "locations.h"

namespace borealis {

using std::multimap;
using std::unordered_map;
using std::less;
using std::binary_function;
using std::pair;
using std::make_pair;
using borealis::Locus;

template<class T>
class location_container {
public:
    typedef multimap< Locus, T > l2t_t;
    typedef unordered_map< T, Locus > t2l_t;

    typedef typename l2t_t::iterator iterator;
    typedef pair<iterator, iterator> range;

    typedef typename l2t_t::const_iterator const_iterator;
    typedef pair<const_iterator, const_iterator> const_range;

private:
    l2t_t l2t;
    t2l_t t2l;

public:
    void put(const Locus& loc, T val) {
        using borealis::util::removeFromMultimap;

        if(t2l.count(val)) {
            removeFromMultimap(l2t, loc, val);
        }
        t2l[val] = loc;
        l2t.insert(make_pair(loc, val));
    }

    bool contains(const Locus& loc) const {
        return l2t.count(loc) > 0;
    }

    bool contains(T val) const {
        return t2l.count(val) > 0;
    }

    const Locus& operator[] (T key) const {
        return t2l.at(key);
    }

    const_range range_after(const Locus& loc) const {
        auto start = l2t.upper_bound(loc);
        auto end = start;
        while(end != l2t.end() && end->first == start->first) ++end;
        return make_pair(start, end);
    }

    const l2t_t& getFrom() const { return l2t; }
    const t2l_t& getTo() const { return t2l; }


};

} // namespace borealis
#endif // SOURCE_LOCATION_TRACKER_LOCATION_CONTAINER_H
