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

#if defined(__GNUC__) && defined(__cplusplus) && (__GNUC__ >= 3) && !defined(_RWSTD_VER) && (!defined(__QNXNTO__) || (defined(__GLIBCXX__) || defined(__GLIBCPP__)))

#include <cxxabi.h>

namespace borealis {
namespace util {

template<class T>
inline std::string demangle(const char* mangled) {
    if ( !mangled ) {
        return "(null)";
    } else if ( !mangled[0] ) {
        return "(empty)";
    }
    int status = -1;

    std::shared_ptr<const char> demangled(
        abi::__cxa_demangle( mangled, 0, 0, &status ),
        [](const char* buf) { free(static_cast<void*>(buf)); }
    );

    if ( !demangled.get() ) {
        return mangled;
    } else if ( 0 == status ) {
        std::string vi{ demangled.get() };
        return vi;
    }
    return mangled;
}

template<class T>
std::string type_name() {
    return demangle(typeid(T).name());
}

template<class T>
std::string type_name(const T& v) {
    return demangle(typeid(v).name());
}

} // namespace util
} // namespace borealis


#else // not GNU C++

namespace borealis {
namespace util {

template<class T>
std::string type_name() {
    return (typeid(T).name());
}

template<class T>
std::string type_name(const T& v) {
    return (typeid(v).name());
}

} // namespace util
} // namespace borealis

#endif // #if defined(compiler trash)



namespace borealis {
namespace util {

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
