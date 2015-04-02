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
{
    return left ^= right + 0x9e3779b9 + (left << 6) + (left >> 2);
}

template<int Index, class ...Types>
struct tuple_hash_impl {
    size_t operator()(size_t a, const std::tuple<Types...>& t) const {
        typedef typename std::tuple_element<Index, std::tuple<Types...>>::type nexttype;
        tuple_hash_impl<Index-1, Types...> next;
        size_t b = std::hash<nexttype>()(std::get<Index>(t));
        return next(hash_combiner(a, b), t);
    }
};

template<class ...Types>
struct tuple_hash_impl<0, Types...> {
    size_t operator()(size_t a, const std::tuple<Types...>& t) const {
        typedef typename std::tuple_element<0, std::tuple<Types...>>::type nexttype;
        size_t b = std::hash<nexttype>()(std::get<0>(t));
        return hash_combiner(a, b);
    }
};

} // namespace impl_

template<class T>
inline size_t simple_hash_value(T&& v) {
    return std::hash<std::decay_t<T>>{}(std::forward<T>(v));
}

template<class H, class H2, class ...T>
inline size_t simple_hash_value(H&& h, H2&& h2, T&&... t) {
    return impl_::hash_combiner(
        simple_hash_value(std::forward<H>(h)),
        simple_hash_value(std::forward<H2>(h2), std::forward<T>(t)...)
    );
}

} // namespace hash
} // namespace util
} // namespace borealis

namespace std {

template<class T>
struct hash< std::vector<T> > {
    std::hash<T> h;
    size_t operator()(const std::vector<T>& vec) const {
        size_t res = 3;
        for (const T& t : vec) {
            res = 17 * res + h(t);
        }
        return res;
    }
};

template<class T>
struct hash< const std::vector<T> > {
    std::hash<T> h;
    size_t operator()(const std::vector<T>& vec) const {
        size_t res = 3;
        for (const T& t : vec) {
            res = 17 * res + h(t);
        }
        return res;
    }
};

template<>
struct hash<std::tuple<>> {
    size_t operator()(std::tuple<>) const { return 42U; }
};

template<class HT, class... Types>
struct hash<std::tuple<HT, Types...>> {
    size_t operator()(const std::tuple<HT, Types...>& t) const {
        constexpr auto begin = sizeof...(Types); // == tuple_size - 1
        return borealis::util::hash::impl_::tuple_hash_impl<begin, HT, Types...>()(59, t);
    }
};

template<class T, class U>
struct hash<std::pair<T, U>> {
    size_t operator()(const std::pair<T, U >& t) const {
        return borealis::util::hash::impl_::tuple_hash_impl<1, T, U>()(59, t);
    }
};

} // namespace std

#endif /* HASH_HPP_ */
