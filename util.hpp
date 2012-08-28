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

template<class K, class V>
bool contains(const std::map<K, V>& map, const K& k) {
	if (map.find(k) != map.end()) return true;
	else return false;
}

} // namespace util



namespace std {

template<class T>
llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const std::vector<T>& v) {
	using namespace::std;

	s << "[";
	if (!v.empty()) {
		s << v.at(0);
		for_each(v.begin() + 1, v.end(), [&s](const T& e){
			s << "," << e;
		});
	}
	s << "]";

	return s;
}

} /* namespace std */

#endif /* UTIL_HPP_ */
