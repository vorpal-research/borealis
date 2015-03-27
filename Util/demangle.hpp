/*
 * demangle.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef DEMANGLE_H
#define DEMANGLE_H

#include <memory>
#include <string>

#include <lib/typesig/typesig.hpp>

namespace borealis{
namespace util {

template<class T>
std::string type_name() {
    return typesig::signature<T>();
}

template<class T>
std::string type_name(const T& v) {
    return typesig::signature<T>();
}

template<class T>
std::string clean_type_name() {
    std::string full = type_name<T>();
    auto nspace = full.find_last_of(':');
    full.erase(0, nspace == std::string::npos ? std::string::npos : nspace+1);
    return full;
}

template<class T>
std::string clean_type_name(const T& v) {
    std::string full = type_name<T>(v);
    auto nspace = full.find_last_of(':');
    full.erase(0, nspace == std::string::npos ? std::string::npos : nspace+1);
    return full;
}

} // namespace util
} // namespace borealis

#endif // DEMANGLE_H
