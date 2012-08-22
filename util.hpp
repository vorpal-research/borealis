/*
 * util.hpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <algorithm>

namespace util {

template<class Container, class Callable>
void for_each(const Container& con, Callable cl) {
	std::for_each(con.begin(), con.end(), cl);
}


} // namespace util


#endif /* UTIL_HPP_ */
