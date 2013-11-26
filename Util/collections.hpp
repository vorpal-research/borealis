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
#include <vector>

#include "Util/iterators.hpp"
#include "Util/type_traits.hpp"

#include "Util/macros.h"

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
    struct defaultPredicate {
        template<class T>
        bool operator()(const T& v) const { return static_cast<bool>(v); }
    };

    CollectionView(const CollectionView&) = default;
    CollectionView(CollectionView&&) = default;
    CollectionView(ContainerIter begin, ContainerIter end) : begin_(begin), end_(end) {}
    CollectionView(const std::pair<ContainerIter, ContainerIter>& iters) : begin_(iters.first), end_(iters.second) {}

    ContainerIter begin() const { return begin_; }
    ContainerIter end() const { return end_; }
    bool empty() const { return begin_ == end_; }

    CollectionView<flattened_iterator<ContainerIter>> flatten() const {
        return view(
            borealis::util::flat_iterator(begin_, end_),
            borealis::util::flat_iterator(end_)
        );
    }

    template<class Mapping>
    CollectionView<mapped_iterator<ContainerIter, Mapping>> map(Mapping mapping) const {
        return view(
            borealis::util::map_iterator(begin_, mapping),
            borealis::util::map_iterator(end_, mapping)
        );
    }

    template<class Pred>
    CollectionView<filtered_iterator<ContainerIter, Pred>> filter(Pred pred) const {
        return view(
            borealis::util::filter_iterator(begin_, end_, pred),
            borealis::util::filter_iterator(end_, pred)
        );
    }

    CollectionView<filtered_iterator<ContainerIter, defaultPredicate>> filter() const {
        return filter(defaultPredicate());
    }

    CollectionView<ContainerIter> drop(size_t n) const {
        auto nbegin = begin_;
        for (auto i = 0U; i < n && nbegin != end_; ++i, ++nbegin);
        return CollectionView{ nbegin, end_ };
    }

    CollectionView<ContainerIter> take(size_t n) const {
        auto nend = begin_;
        for (auto i = 0U; i < n && nend != end_; ++i, ++nend);
        return CollectionView{ begin_, nend };
    }

    template<class OtherIter>
    CollectionView<zipping_iterator<ContainerIter, OtherIter>> operator^ (const CollectionView<OtherIter>& that) const {
        return view(
            zip(begin_, that.begin()),
            zip(end_, that.end())
        );
    }

    template<class Con>
    Con to() {
        return Con(begin_, end_);
    }

    typedef typename std::iterator_traits<ContainerIter>::value_type value_type;

    std::list<value_type> toList() {
        return to<std::list<value_type>>();
    }

    std::vector<value_type> toVector() {
        return to<std::vector<value_type>>();
    }

    std::set<value_type> toSet() {
        return to<std::set<value_type>>();
    }

};

////////////////////////////////////////////////////////////////////////////////

struct deref {
    template<class T>
    auto operator()(T& val) const -> decltype(*val) {
        return *val;
    }

    template<class T>
    auto operator()(const T& val) const -> decltype(*val) {
        return *val;
    }
};

struct takePtr {
    template<class T>
    auto operator()(T& val) const -> decltype(&val)  {
        return &val;
    }

    template<class T>
    auto operator()(const T& val) const -> decltype(&val) {
        return &val;
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class C> inline auto head(const C& con) QUICK_RETURN(*std::begin(con));

template<class C> inline auto head(C& con) QUICK_RETURN(*std::begin(con));

template<class Iter>
inline auto view(Iter b, Iter e) -> CollectionView<Iter> {
    return CollectionView<Iter>(b, e);
}

template<class Iter>
inline auto view(const std::pair<Iter, Iter>& is) -> CollectionView<Iter> {
    return CollectionView<Iter>(is);
}


template<class Container>
inline auto viewContainer(const Container& con) -> CollectionView<decltype(std::begin(con))> {
    return view( std::begin(con), std::end(con) );
}

template<class Container>
inline auto viewContainer(const Container* con) -> decltype(viewContainer(*con)) {
    return viewContainer(*con);
}

template<class Container>
inline auto viewContainer(Container& con) -> CollectionView<decltype(std::begin(con))> {
    return view( std::begin(con), std::end(con) );
}

template<class Container>
inline auto viewContainer(Container* con) -> decltype(viewContainer(*con)) {
    return viewContainer(*con);
}


template<class Container>
inline auto reverse(Container& con) -> CollectionView<decltype(con.rbegin())> {
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
inline auto take(unsigned int count, const Container& con) -> decltype(viewContainer(con)) {
    return viewContainer(con).take(count);
}

template<class Container>
inline auto take(unsigned int count, Container& con) -> decltype(viewContainer(con)) {
    return viewContainer(con).take(count);
}


template<class Elem>
inline auto range(const Elem& from, const Elem& to) ->
        CollectionView<counting_iterator<Elem>> {
    return view(
        counting_iterator<Elem>(from),
        counting_iterator<Elem>(to)
    );
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
T copy(T other) { return other; }

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

template<class Container, class T>
bool contains(const Container& con, const T& t) {
    auto end = std::end(con);
    return std::find(std::begin(con), end, t) != end;
}

template<class M, class K>
auto at(M&& m, K&& k) -> decltype(justRef(std::forward<M>(m).at(std::forward<K>(k)))) {
    auto it = m.find(std::forward<K>(k));
    if(it != std::end(std::forward<M>(m))) {
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

#include "Util/unmacros.h"

#endif /* COLLECTIONS_HPP_ */
