/*
 * util.hpp
 *
 *  Created on: Aug 23, 2012
 *      Author: ice-phoenix
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <llvm/Support/raw_ostream.h>

#include <map>
#include <utility>

template<class K, class V>
bool contains(const std::map<K, V>& map, const K& k) {
	if (map.find(k) != map.end()) return true;
	else return false;
}

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
