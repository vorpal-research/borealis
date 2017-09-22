/*
 * algorithm.hpp
 *
 *  Created on: Jan 22, 2014
 *      Author: belyaev
 */

#ifndef ALGORITHM_HPP_
#define ALGORITHM_HPP_

#include <algorithm>

namespace borealis {
namespace util {

#define BE(C) std::begin(C), std::end(C)

template<class C, class Pred>
bool all_of(const C& con, Pred pred) {
    return std::all_of(BE(con), pred);
}

template<class C, class Pred>
bool any_of(const C& con, Pred pred) {
    return std::any_of(BE(con), pred);
}

template<class C, class T>
size_t count(const C& con, const T& val) {
    return std::count(BE(con), val);
}

template<class C, class Pred>
size_t count_if(const C& con, Pred pred) {
    return std::count_if(BE(con), pred);
}

template<class C, class V>
decltype(std::begin(std::declval<C>())) find(const C& c, const V& v) {
    return std::find(BE(c), v);
}

template<class C, class Pred>
decltype(std::begin(std::declval<C>())) find_if(const C& c, Pred p) {
    return std::find_if(BE(c), p);
}

template<class C, class F>
void for_each(const C& c, F f) {
    std::for_each(BE(c), f);
}

#undef BE

template <typename T>
T min(const T& t) {
    return t;
}

template <typename Head, typename ...Tail>
Head min(const Head& h, const Tail&... t) {
    auto tmin = min<Tail...>(t...);
    return h < tmin ? h : tmin;
};

template <typename T>
T max(const T& t) {
    return t;
}

template <typename Head, typename ...Tail>
Head max(const Head& h, const Tail&... t) {
    auto tmax = max<Tail...>(t...);
    return h < tmax ? tmax : h;
};

} /* namespace util */
} /* namespace borealis */

#endif /* ALGORITHM_HPP_ */
