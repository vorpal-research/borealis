/*
 * util.hpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <algorithm>
#include <list>

namespace util {

template<class Container, class Callable>
void for_each(const Container& con, Callable cl) {
	std::for_each(con.begin(), con.end(), cl);
}

template<class T>
T copy(T other) { return other; }

template<class T, class Pred>
std::list<T> filter_not(std::list<T>&& lst, Pred pred) {
	auto rem = std::remove_if(lst.begin(), lst.end(), pred);
	lst.erase(rem, lst.end());
	return lst;
}

template<class T, class Pred>
std::list<T> filter_not(const std::list<T>& lst, Pred pred) {
	return filter_not(copy(lst), pred);
}




} // namespace util


#endif /* UTIL_HPP_ */
