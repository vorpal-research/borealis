/*
 * hash.hpp
 *
 *  Created on: Feb 1, 2013
 *      Author: ice-phoenix
 */

#ifndef HASH_HPP_
#define HASH_HPP_

#include <utility>
#include <vector>

namespace borealis {
namespace util {
namespace hash {

template<int A, int B, typename T>
size_t get_hash_code(T& t) {
    return B * A + std::hash<T>()(t);
}

template<int A, int B, typename Head, typename ...Tail>
size_t get_hash_code(Head& hd, Tail& ...tl) {
    return B * get_hash_code<A, B, Tail...>(tl...) + std::hash<Head>()(hd);
}

template<int A, int B, typename T>
size_t get_hash_code(T&& t) {
    return B * A + std::hash<T>()(t);
}

template<int A, int B, typename Head, typename ...Tail>
size_t get_hash_code(Head&& hd, Tail&& ...tl) {
    return B * get_hash_code<A, B, Tail...>(tl...) + std::hash<Head>()(hd);
}

template<int A, int B>
struct hasher {
    template<typename ...Args>
    size_t operator()(Args&... args) {
        return get_hash_code<A, B, Args...>(args...);
    }
};



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

} // namespace hash
} // namespace util
} // namespace borealis

namespace std {

template<class T>
struct hash< std::vector<T> > {
    size_t operator()(const std::vector<T>& vec) const {
        size_t res = 3;
        for (const T& t : vec) {
            res = 17 * res + std::hash<T>()(t);
        }
        return res;
    }
};

template<class T>
struct hash< const std::vector<T> > {
    size_t operator()(const std::vector<T>& vec) const {
        size_t res = 3;
        for (const T& t : vec) {
            res = 17 * res + std::hash<T>()(t);
        }
        return res;
    }
};

template<class...types>
struct hash<std::tuple<types...>> {
    size_t operator()(const std::tuple<types...>& t) const {
        const size_t begin = std::tuple_size<std::tuple<types...>>::value-1;
        return borealis::util::hash::impl_::hash_impl<begin, types...>()(59, t);
    }
};

template<class T, class U>
struct hash<std::pair<T, U>> {
    size_t operator()(const std::pair<T, U >& t) const {
        const size_t begin = 1;
        return borealis::util::hash::impl_::hash_impl<begin, T, U>()(59, t);
    }
};

} // namespace std

#endif /* HASH_HPP_ */
