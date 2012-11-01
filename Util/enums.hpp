/*
 * enums.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef ENUMS_HPP_
#define ENUMS_HPP_

namespace borealis {
namespace util {
namespace enums {

template<typename Enum>
inline typename std::underlying_type<Enum>::type asInteger(Enum e) {
    return static_cast<typename std::underlying_type<Enum>::type>(e);
}

// generalized hash for enum classes
template<class Enum>
struct enum_hash {
    typedef typename std::underlying_type<Enum>::type raw;
    std::hash<raw> delegate;

    inline size_t operator()( Enum e ) const {
        return delegate(asInteger(e));
    }
};

} // namespace enums
} // namespace util
} // namespace borealis

#endif /* ENUMS_HPP_ */
