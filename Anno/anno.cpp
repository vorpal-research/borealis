/*
 * anno.cpp
 *
 *  Created on: Oct 5, 2012
 *      Author: belyaev
 */

#include "Anno/anno.h"
#include "Anno/anno.hpp"

namespace borealis {
namespace anno {

std::vector<command> parse(const std::string& v) {
    return calculator::parse_command<location>(v);
}

prod_t parseTerm(const std::string& v) {
    return calculator::parse_term<location>(v);
}

std::vector<command> parse(const char* vbegin, const char* vend) {
    return calculator::parse_command<const char*, location>(std::make_pair(vbegin, vend));
}
prod_t parseTerm(const char* vbegin, const char* vend) {
    return calculator::parse_term<const char*, location>(std::make_pair(vbegin, vend));
}

} // namespace anno
} // namespace borealis
