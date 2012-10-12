/*
 * command.hpp
 *
 *  Created on: Apr 2, 2012
 *      Author: belyaev
 */

#ifndef COMMAND_HPP_
#define COMMAND_HPP_

#include "production.h"
#include <string>
#include <list>
#include <algorithm>
using std::for_each;
#include <iostream>
using std::basic_ostream;

namespace borealis{
namespace anno{

using std::move;

struct command {
	std::string name_;
	std::list<prod_t> args_;
};

template<class Char>
basic_ostream<Char>& operator<<(basic_ostream<Char>& ost, const command& com) {
	ost << com.name_ << "(";
	for_each(com.args_.begin(), --com.args_.end(),[&ost](const prod_t& args){
		ost << *args << ",";
	});
	ost << *(com.args_.back()) << ")";
	return ost;
}

} //namespace anno
} //namespace borealis

#endif /* COMMAND_HPP_ */
