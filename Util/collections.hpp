/*
 * collections.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef COLLECTIONS_HPP_
#define COLLECTIONS_HPP_

#include <algorithm>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Util/iterators.hpp"
#include "Util/option.hpp"
#include "Util/type_traits.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

template<class It>
using CollectionView = iterator_view<It>;

////////////////////////////////////////////////////////////////////////////////

template<class C> inline auto head(const C& con) QUICK_RETURN(*std::begin(con));

template<class C> inline auto head(C& con) QUICK_RETURN(*std::begin(con));


template<class Container>
inline auto reverse(Container& con) -> iterator_view<decltype(con.rbegin())> {
    return view( con.rbegin(), con.rend() );
}

template<class Container>
inline auto reverse(Container* c) -> decltype(reverse(*c)) {
    return reverse(*c);
}


template<class Container>
inline auto tail(const Container& con) -> decltype(viewContainer(con)) {
    return viewContainer(con).drop(1);
}

template<class Container>
inline auto tail(const Container* con) -> decltype(tail(*con)) {
    return tail(*con);
}

template<class Container>
inline auto tail(Container& con) -> decltype(viewContainer(con)) {
    return viewContainer(con).drop(1);
}

template<class Container>
inline auto tail(Container* con) -> decltype(tail(*con)) {
    return tail(*con);
}


template<class Container>
inline auto take(size_t count, const Container& con) -> decltype(viewContainer(con)) {
    return viewContainer(con).take(count);
}

template<class Container>
inline auto take(size_t count, Container& con) -> decltype(viewContainer(con)) {
    return viewContainer(con).take(count);
}

static struct {
    template<class K, class V>
    const K& operator()(const std::pair<K, V>& pr) const {
        return pr.first;
    }
} get_key;

static struct {
    template<class K, class V>
    const V& operator()(const std::pair<K, V>& pr) const {
        return pr.second;
    }
    template<class K, class V>
    V& operator()(std::pair<K, V>& pr) const {
        return pr.second;
    }
} get_value;

template<class Container>
inline auto viewContainerValues(const Container& con) {
    return viewContainer(con).map(get_value);
}

template<class Container>
inline auto viewContainerValues(Container& con) {
    return viewContainer(con).map(get_value);
}

template<class Container>
inline auto viewContainerKeys(const Container& con) {
    return viewContainer(con).map(get_key);
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
T copy(const T& other) { return other; }

template<class T, class Pred>
std::list<T> filter_not(std::list<T>&& lst, const Pred pred) {
    auto begin = std::begin(lst);
    auto end = std::end(lst);
    lst.erase(std::remove_if(begin, end, pred), end);
    return lst;
}

template<class T, class Pred>
std::list<T> filter_not(const std::list<T>& lst, const Pred pred) {
    return filter_not(copy(lst), pred);
}

template<class K, class _>
bool containsKey(const std::map<K, _>& map, add_const_reference_t<K> k) {
    return map.find(k) != map.end();
}

template<class K, class _>
bool containsKey(const std::unordered_map<K, _>& map, add_const_reference_t<K> k) {
    return map.find(k) != map.end();
}

template<class K, class V>
void removeFromMultimap(std::multimap<K, V>& map, add_const_reference_t<K> k, add_const_reference_t<V> v) {
    auto range = map.equal_range(k);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class K, class V>
void removeFromMultimap(std::unordered_multimap<K, V>& map, add_const_reference_t<K> k, add_const_reference_t<V> v) {
    auto range = map.equal_range(k);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class K, class V, class KK>
auto less_or_equal(const std::map<K, V>& map, KK&& k) {
    if (map.empty()) return std::end(map);

    auto&& ub = map.upper_bound(std::forward<KK>(k));
    if (ub == std::begin(map)) return std::end(map);
    else return std::prev(ub);
}

namespace impl_ {

template<class Container, typename T>
static sfinaer_t<std::true_type, decltype(std::declval<Container>().find(std::declval<T>()))>
has_find(int) { return std::true_type{}; };

template<class Container, typename T>
static std::false_type
has_find(long) { return std::false_type{}; };

template<class Container, typename T, typename SFINAE = void>
struct contains_with_find {
    bool doit(const Container& con, const T& t, std::true_type) const {
        auto&& end = std::end(con);
        return con.find(t) != end;
    }
    bool doit(const Container& con, const T& t, std::false_type) const {
        auto&& end = std::end(con);
        return std::find(std::begin(con), end, t) != end;
    }
    bool operator()(const Container& con, const T& t) const {
        return doit(con, t, has_find<Container, T>(0));
    }
};

} // namespace impl_

template<class Container, typename T>
bool contains(const Container& con, const T& t) {
    return impl_::contains_with_find<Container, T>()(con, t);
}

template<class M, class K>
auto at(M&& m, K&& k) -> decltype(justRef(std::forward<M>(m).at(std::forward<K>(k)))) {
    auto it = m.find(k);
    if(it != std::end(m)) {
        return justRef(std::forward<M>(m).at(std::forward<K>(k)));
    }
    return nothing();
}

template<class C1, class C2, class BinaryPred>
bool equal(const C1& c1, const C2& c2, BinaryPred pred) {
    auto c1beg = std::begin(c1), c1end = std::end(c1);
    auto c2beg = std::begin(c2), c2end = std::end(c2);
    for( ; c1beg != c1end && c2beg != c2end ; ++c1beg, ++c2beg) {
        if (!pred(*c1beg, *c2beg)) return false;
    }
    return c1beg == c1end && c2beg == c2end;
}

template<class Container>
auto max_element(Container&& c) -> decltype(*c.begin()) {
    return *std::max_element(std::forward<Container>(c).begin(), std::forward<Container>(c).end());
}

////////////////////////////////////////////////////////////////////////////////

namespace impl_ {

template<class Tup, class Op, size_t N = std::tuple_size<Tup>::value>
struct tuple_for_each_helper {
    enum{ tsize = std::tuple_size<Tup>::value };
    inline static void apply(Tup&& tup, Op op) {
        op(std::get<tsize-N>(std::forward<Tup>(tup)));
        tuple_for_each_helper<Tup, Op, N-1>::apply(std::forward<Tup>(tup), op);
    }
};

template<class Tup, class Op>
struct tuple_for_each_helper<Tup, Op, 0> {
    inline static void apply(Tup&&, Op) {};
};

} // namespace impl_

template<class Tup, class Op>
void tuple_for_each(Tup&& tup, Op op) {
    typedef impl_::tuple_for_each_helper<Tup, Op> delegate;
    return delegate::apply(std::forward<Tup>(tup), op);
}

} // namespace util
} // namespace borealis

////////////////////////////////////////////////////////////////////////////////

namespace std {

template<typename T>
std::vector<T> operator+ (const std::vector<T>& a, const std::vector<T>& b) {
    std::vector<T> res;
    res.reserve(a.size() + b.size());
    res.insert(res.end(), a.begin(), a.end());
    res.insert(res.end(), b.begin(), b.end());
    return res;
}

} // namespace std

#include "Util/unmacros.h"

#endif /* COLLECTIONS_HPP_ */
