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
#include <map>

namespace borealis {

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




namespace streams {

template<class T>
struct error_printer {
	const T& val;
	error_printer(const T& v): val(v) {}
};

template<class T>
llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const error_printer<T>& v) {
	using namespace::std;

	s.changeColor(s.RED);

	s << v.val;

	s.resetColor();

	return s;
}

// prints value in red
// used as
// errs() << error(22) << endl;
template<class T>
error_printer<T> error(const T& val) { return error_printer<T>(val); }

} /* namespace streams */

} /* namespace util */

} /* namespace borealis */

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
