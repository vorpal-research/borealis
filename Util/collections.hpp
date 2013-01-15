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
#include <unordered_map>
#include <set>
#include <tuple>
#include <vector>

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
    CollectionView(ContainerIter begin, ContainerIter end): begin_(begin), end_(end) {
    }
    CollectionView(std::pair<ContainerIter, ContainerIter> iters): begin_(iters.first), end_(iters.second) {
    }

    ContainerIter begin() const { return begin_; }
    ContainerIter end() const { return end_; }
    bool empty() const { return begin_ == end_; }
};

template<class Container>
inline auto head(const Container& con) -> decltype(*con.begin()) {
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
inline auto tail(const Container& con) -> CollectionView<decltype(con.begin())> {
    return view(++con.begin(), con.end());
}

template<class Container, class Callable>
inline void for_each(const Container& con, const Callable cl) {
    std::for_each(con.begin(), con.end(), cl);
}

template<class Container, class Callable>
inline void for_each(Container& con, const Callable cl) {
    std::for_each(con.begin(), con.end(), cl);
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
    if (map.find(k) != map.end()) return true;
    else return false;
}

template<class K, class V>
void removeFromMultimap(std::multimap<K,V>& map, const K& k, const V& v) {
    auto range = map.equal_range(k);

    for(auto it = range.first; it != range.second; ++it) {
        if(it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class K, class V>
void removeFromMultimap(std::unordered_multimap<K,V>& map, const K& k, const V& v) {
    auto range = map.equal_range(k);

    for(auto it = range.first; it != range.second; ++it) {
        if(it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class Container, class T>
bool contains(const Container& con, const T& t) {
    if (std::find(con.begin(), con.end(), t) != con.end()) return true;
    else return false;
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

namespace impl_ {
namespace {

inline static size_t hash_combiner(size_t left, size_t right) // replaceable
{ return left^right; }

template<int index, class...types>
struct hash_impl {
    size_t operator()(size_t a, const std::tuple<types...>& t) const {
        typedef typename std::tuple_element<index, std::tuple<types...>>::type nexttype;
        hash_impl<index-1, types...> next;
        size_t b = std::hash<nexttype>()(std::get<index>(t));
        return next(hash_combiner(a, b), t);
    }
};

template<class...types>
struct hash_impl<0, types...> {
    size_t operator()(size_t a, const std::tuple<types...>& t) const {
        typedef typename std::tuple_element<0, std::tuple<types...>>::type nexttype;
        size_t b = std::hash<nexttype>()(std::get<0>(t));
        return hash_combiner(a, b);
    }
};

} // namespace
} // namespace impl_

} // namespace util
} // namespace borealis



namespace std {

template<class...types>
struct hash<std::tuple<types...>> {
    size_t operator()(const std::tuple<types...>& t) const {
        const size_t begin = std::tuple_size<std::tuple<types...>>::value-1;
        return borealis::util::impl_::hash_impl<begin, types...>()(1, t); // 1 should be some larger value
    }
};

template<class T, class U>
struct hash<std::pair<T, U>> {
    size_t operator()(const std::pair<T, U >& t) const {
        const size_t begin = 1;
        return borealis::util::impl_::hash_impl<begin, T, U>()(1, t); // 1 should be some larger value
    }
};

} // namespace std

#endif /* COLLECTIONS_HPP_ */
