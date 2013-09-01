/*
 * string_cast.hpp
 *
 *  Created on: Aug 15, 2013
 *      Author: belyaev
 */

#ifndef STRING_CAST_HPP_
#define STRING_CAST_HPP_

#include <string>

#include "Util/streams.hpp"

#include "Util/macros.h"

namespace borealis {
namespace anno {

// bad string cast exception, no additional info provided, sorry =)
class bad_string_cast : public std::exception {};

// the casting function itself
// throws bad_string_cast
template<class T>
T string_cast(const std::string& sexpr) {
    auto ret = borealis::util::fromString<T>(sexpr);
    if(!ret) throw bad_string_cast();
    else return ret.getUnsafe();
}

} // namespace anno
} // namespace borealis

#include "Util/unmacros.h"

#endif /* STRING_CAST_HPP_ */
