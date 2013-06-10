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
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "Util/iterators.hpp"

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
};

////////////////////////////////////////////////////////////////////////////////

struct isValid {
    template<class T>
    bool operator()(const T& val) const {
        return val ? true : false;
    }
};

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
inline auto view(const std::pair<Iter,Iter>& is) -> CollectionView<Iter> {
    return CollectionView<Iter>(is);
}

template<class Container>
inline auto viewContainer(const Container& con) -> CollectionView<decltype(std::begin(con))> {
    return CollectionView<decltype(std::begin(con))>{ std::begin(con), std::end(con) };
}

template<class Container>
inline auto viewContainer(const Container* con) -> decltype(viewContainer(*con)) {
    return viewContainer(*con);
}

template<class Container>
inline auto viewContainer(Container& con) -> CollectionView<decltype(std::begin(con))> {
    return CollectionView<decltype(std::begin(con))>{ std::begin(con), std::end(con) };
}

template<class Container>
inline auto viewContainer(Container* con) -> decltype(viewContainer(*con)) {
    return viewContainer(*con);
}

template<class Container>
inline auto reverse(Container& c) -> CollectionView<decltype(c.rbegin())> {
    return view(c.rbegin(), c.rend());
}

template<class Container>
inline auto reverse(Container* c) -> decltype(reverse(*c)) {
    return reverse(*c);
}

template<class Container>
inline auto tail(const Container& con) -> CollectionView<decltype(std::begin(con))> {
    // fix the issue for initializer_list in which begin() is not assignable
    // with std::next
    return view(std::next(std::begin(con)), std::end(con));
}

template<class Container>
inline auto tail(Container& con) -> CollectionView<decltype(std::begin(con))> {
    // fix the issue for initializer_list in which begin() is not assignable
    // with std::next
    return view(std::next(std::begin(con)), std::end(con));
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

template<class T>
using add_const_ref_q =
    typename std::add_lvalue_reference<
        typename std::add_const<T>::type
    >::type;

template<class K, class _>
bool containsKey(const std::map<K, _>& map, add_const_ref_q<K> k) {
    return map.find(k) != map.end();
}

template<class K, class _>
bool containsKey(const std::unordered_map<K, _>& map, add_const_ref_q<K> k) {
    return map.find(k) != map.end();
}

template<class K, class V>
void removeFromMultimap(std::multimap<K, V>& map, add_const_ref_q<K> k, add_const_ref_q<V> v) {
    auto range = map.equal_range(k);

    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class K, class V>
void removeFromMultimap(std::unordered_multimap<K, V>& map, add_const_ref_q<K> k, add_const_ref_q<V> v) {
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

////////////////////////////////////////////////////////////////////////////////

namespace impl_ {
namespace {

template<class Tup, class Op, size_t N = std::tuple_size<Tup>::value>
struct tuple_for_each_helper {
    enum{ tsize = std::tuple_size<Tup>::value };
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

#include "Util/unmacros.h"

#endif /* COLLECTIONS_HPP_ */
