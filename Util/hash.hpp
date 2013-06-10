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
size_t get_hash_code(const T& t) {
    return B * A + std::hash<T>()(t);
}

template<int A, int B, typename Head, typename ...Tail>
size_t get_hash_code(const Head& hd, const Tail& ...tl) {
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
    size_t operator()(const Args&... args) const {
        return get_hash_code<A, B, Args...>(args...);
    }
};

typedef hasher<3, 17> defaultHasher;



namespace impl_ {

inline static size_t hash_combiner(size_t left, size_t right) // replaceable
{ return left^right; }

template<int Index, class ...Types>
struct hash_impl {
    size_t operator()(size_t a, const std::tuple<Types...>& t) const {
        typedef typename std::tuple_element<Index, std::tuple<Types...>>::type nexttype;
        hash_impl<Index-1, Types...> next;
        size_t b = std::hash<nexttype>()(std::get<Index>(t));
        return next(hash_combiner(a, b), t);
    }
};

template<class ...Types>
struct hash_impl<0, Types...> {
    size_t operator()(size_t a, const std::tuple<Types...>& t) const {
        typedef typename std::tuple_element<0, std::tuple<Types...>>::type nexttype;
        size_t b = std::hash<nexttype>()(std::get<0>(t));
        return hash_combiner(a, b);
    }
};

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

template<class... Types>
struct hash<std::tuple<Types...>> {
    size_t operator()(const std::tuple<Types...>& t) const {
        const size_t begin = std::tuple_size<std::tuple<Types...>>::value-1;
        return borealis::util::hash::impl_::hash_impl<begin, Types...>()(59, t);
    }
};

template<class T, class U>
struct hash<std::pair<T, U>> {
    size_t operator()(const std::pair<T, U >& t) const {
        return borealis::util::hash::impl_::hash_impl<1, T, U>()(59, t);
    }
};

} // namespace std

#endif /* HASH_HPP_ */
