/*
 * enums.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef ENUMS_HPP_
#define ENUMS_HPP_

#include "Util/type_traits.hpp"

namespace borealis {
namespace util {
namespace enums {

template<typename Enum>
inline underlying_type_t<Enum> asInteger(Enum e) {
    return static_cast< underlying_type_t<Enum> >(e);
}

// generalized hash for enum classes
template<class Enum>
struct enum_hash {
    typedef underlying_type_t<Enum> raw;
    std::hash<raw> delegate;

    inline size_t operator()( Enum e ) const {
        return delegate(asInteger(e));
    }
};

template<class T>
auto enumValues() -> decltype(enumValuesImpl(T())) {
    return enumValuesImpl(T());
}

} // namespace enums
} // namespace util
} // namespace borealis

#endif /* ENUMS_HPP_ */
