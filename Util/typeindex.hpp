/*
 * typeindex.hpp
 *
 *  Created on: Oct 22, 2012
 *      Author: belyaev
 */

#ifndef TYPEINDEX_HPP_
#define TYPEINDEX_HPP_

#include <unordered_map>
#include <utility>

namespace borealis {

typedef std::size_t id_t;

template<class T>
struct type_index {
    constexpr static id_t id() {
        static_assert(sizeof(&id) <= sizeof(id_t), "id type not sufficient =(");

        return reinterpret_cast<id_t>(&id);
    }
};

template<class T>
constexpr id_t type_id(const T&) { return type_index<T>::id(); }

template<class T>
constexpr id_t type_id() { return type_index<T>::id(); }

} // namespace borealis

namespace std {
    template<class T>
    struct hash<borealis::type_index<T>> {
        size_t operator()(const borealis::type_index<T>&) {
            return borealis::type_index<T>::id();
        }
    };
} // namespace std

#endif /* TYPEINDEX_HPP_ */
