/*
 * hash.hpp
 *
 *  Created on: Feb 1, 2013
 *      Author: ice-phoenix
 */

#ifndef HASH_HPP_
#define HASH_HPP_

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

} // namespace hash
} // namespace util
} // namespace borealis

#endif /* HASH_HPP_ */
