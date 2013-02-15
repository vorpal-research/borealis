/*
 * anno.cpp
 *
 *  Created on: Oct 5, 2012
 *      Author: belyaev
 */

#include "anno.h"
#include "anno.hpp"

namespace borealis {
namespace anno {

using anno::calculator::parse_command;
using anno::command;
using anno::location;

std::vector<command> parse(const std::string& v) {
    return parse_command<location>(v);
}

} // namespace anno
} // namespace borealis
