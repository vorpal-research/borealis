/*
 * anno.cpp
 *
 *  Created on: Oct 5, 2012
 *      Author: belyaev
 */

#include "anno.h"
#include "command.hpp"

namespace borealis {
namespace anno{

using anno::command;
using anno::location;
using anno::calculator::parse_command;

std::vector<command> parse(std::string v) {
	return parse_command<location>(v);
}

}
}


