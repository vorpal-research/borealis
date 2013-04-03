/*
 * collections.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef COLLECTIONS_HPP_
#define COLLECTIONS_HPP_

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "iterators.hpp"

namespace borealis {
namespace util {

// stupid straightforward collection view
//  - cannot be used after any operation with begin or end
//  - should be used in-place only, like this:
//      int arr[] = { 0,1,2,3,4,5,6,7,8,9,10 };
//      for_each(CollectionView<int*>(arr, arr+10), [](const Elem& el){ ... });
template<class ContainerIter>
class CollectionView {
    ContainerIter begin_;
    ContainerIter end_;

public:
    CollectionView(ContainerIter begin, ContainerIter end) : begin_(begin), end_(end) {}
    CollectionView(std::pair<ContainerIter, ContainerIter> iters) : begin_(iters.first), end_(iters.second) {}

    ContainerIter begin() const { return begin_; }
    ContainerIter end() const { return end_; }
    bool empty() const { return begin_ == end_; }

    CollectionView<flattened_iterator<ContainerIter>> flatten() const {
        return CollectionView<flattened_iterator<ContainerIter>>{flat_iterator(begin_, end_), flat_iterator(end_)};
    }

    template<class Mapping>
    CollectionView<mapped_iterator<ContainerIter, Mapping>> map(Mapping mapping) const {
        return CollectionView<mapped_iterator<ContainerIter, Mapping>>{borealis::util::map_iterator(begin_, mapping), borealis::util::map_iterator(end_, mapping)};
    }

    template<class Pred>
    CollectionView<filtered_iterator<ContainerIter, Pred>> filter(Pred pred) const {
        return CollectionView<filtered_iterator<ContainerIter, Pred>>{filter_iterator(begin_, end_, pred), filter_iterator(end_, pred)};
    }

    static bool defaultPred(decltype(*begin_) v) {
        if(v) return true;
        else return false;
    }

#include "macros.h"
    auto filter() QUICK_CONST_RETURN(this->filter(defaultPred));
#include "unmacros.h"

};


struct deref{
    template<class T>
    auto operator()(T& val) const -> decltype(*val) {
        return *val;
    }

    template<class T>
    auto operator()(const T& val) const -> decltype(*val) {
        return *val;
    }
};

struct takePtr{
    template<class T>
    auto operator()(T& val) const -> decltype(&val)  {
        return &val;
    }

    template<class T>
    auto operator()(const T& val) const -> decltype(&val) {
        return &val;
    }
} ;

template<class Container>
inline auto head(const Container& con) -> decltype(*con.begin()) {
    return *con.begin();
}

template<class Container>
inline auto head(Container& con) -> decltype(*con.begin()) {
    return *con.begin();
}

template<class Iter>
inline auto view(Iter b, Iter e) -> CollectionView<Iter> {
    return CollectionView<Iter>(b, e);
}

template<class Iter>
inline auto view(const std::pair<Iter,Iter>& is) -> CollectionView<Iter> {
    return CollectionView<Iter>(is);
}

template<class Container>
inline auto viewContainer(const Container* con) -> CollectionView<decltype(con->begin())> {
    return CollectionView<decltype(con->begin())>{ con->begin(), con->end() };
}

template<class Container>
inline auto viewContainer(const Container& con) -> CollectionView<decltype(con.begin())> {
    return CollectionView<decltype(con.begin())>{ con.begin(), con.end() };
}

template<class Container>
inline auto viewContainer(Container* con) -> CollectionView<decltype(con->begin())> {
    return CollectionView<decltype(con->begin())>{ con->begin(), con->end() };
}

template<class Container>
inline auto viewContainer(Container& con) -> CollectionView<decltype(con.begin())> {
    return CollectionView<decltype(con.begin())>{ con.begin(), con.end() };
}

template<class Container>
inline auto reverse(Container& c) -> CollectionView<decltype(c.rbegin())> {
    return view(c.rbegin(), c.rend());
}

template<class Container>
inline auto tail(const Container& con) -> CollectionView<decltype(con.begin())> {
    return view(++con.begin(), con.end());
}

template<class Container>
inline auto tail(Container& con) -> CollectionView<decltype(con.begin())> {
    return view(++con.begin(), con.end());
}

template<class T>
T copy(T other) { return other; }

template<class T, class Pred>
std::list<T> filter_not(const std::list<T>&& lst, const Pred pred) {
    auto rem = std::remove_if(lst.begin(), lst.end(), pred);
    lst.erase(rem, lst.end());
    return lst;
}

template<class T, class Pred>
std::list<T> filter_not(const std::list<T>& lst, const Pred pred) {
    return filter_not(copy(lst), pred);
}

template<class K, class _>
bool containsKey(const std::map<K, _>& map, const K& k) {
    return map.find(k) != map.end();
}

template<class K, class _>
bool containsKey(const std::unordered_map<K, _>& map, const K& k) {
    return map.find(k) != map.end();
}

template<class K, class V>
void removeFromMultimap(std::multimap<K,V>& map, const K& k, const V& v) {
    auto range = map.equal_range(k);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class K, class V>
void removeFromMultimap(std::unordered_multimap<K,V>& map, const K& k, const V& v) {
    auto range = map.equal_range(k);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class Container, class T>
bool contains(const Container& con, const T& t) {
    return std::find(con.begin(), con.end(), t) != con.end();
}

namespace impl_ {
namespace {

template<class Tup, class Op, size_t N = std::tuple_size<Tup>::value>
struct tuple_for_each_helper {
    enum { tsize = std::tuple_size<Tup>::value };
    inline static void apply(Tup& tup, Op op) {
        op(std::get<tsize-N>(tup));
        tuple_for_each_helper<Tup, Op, N-1>::apply(tup, op);
    }
};

template<class Tup, class Op>
struct tuple_for_each_helper<Tup, Op, 0> {
    inline static void apply(Tup&, Op) {};
};

} // namespace
} // namespace impl_

template<class Tup, class Op>
void tuple_for_each(Tup& tup, Op op) {
    typedef impl_::tuple_for_each_helper<Tup, Op> delegate;
    return delegate::apply(tup, op);
}

} // namespace util
} // namespace borealis

#endif /* COLLECTIONS_HPP_ */
