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
#include <set>
#include <sstream>
#include <type_traits>
#include <vector>

namespace borealis {



namespace util {

template<class Container, class Callable>
void for_each(const Container& con, const Callable cl) {
	std::for_each(con.begin(), con.end(), cl);
}

template<class T>
T copy(T other) { return other; }

template<class T, class Pred>
std::list<T> filter_not(const std::list<T>&& lst, const Pred pred) {
	auto rem = std::remove_if(lst.begin(), lst.end(), pred);
	lst.erase(rem, lst.end());
	return lst;
}

template<class T, class Pred>
std::list<T> filter_not(const std::list<T>& lst, const Pred pred) {
	return filter_not(copy(lst), pred);
}

template<class K, class _>
bool containsKey(const std::map<K, _>& map, const K& k) {
	if (map.find(k) != map.end()) return true;
	else return false;
}

template<class Container, class T>
bool contains(const Container& con, const T& t) {
	if (std::find(con.begin(), con.end(), t) != con.end()) return true;
	else return false;
}

template<class T>
std::string toString(const T& t) {
	std::ostringstream oss;
	oss << t;
	return oss.str();
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

// prints values in red:
//   errs() << error(42) << endl;
template<class T>
error_printer<T> error(const T& val) { return error_printer<T>(val); }

} /* namespace streams */
} /* namespace util */
} /* namespace borealis */

namespace std {

// Custom output routines and such...

const std::string VECTOR_LEFT_BRACE = "[";
const std::string VECTOR_RIGHT_BRACE = "]";
const std::string SET_LEFT_BRACE = "(";
const std::string SET_RIGHT_BRACE = ")";
const std::string ELEMENT_DELIMITER = ", ";
const std::string NULL_REPR = "<NULL>";

template<typename T>
llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const std::vector<T>& vec) {
	typedef typename std::vector<T>::const_iterator ConstIter;

	using namespace::std;

	s << VECTOR_LEFT_BRACE;
	if (!vec.empty()) {
		ConstIter iter = vec.begin();
		const T el = *iter++;
		s << el;
		for_each(iter, vec.end(), [&s](const T& e){
			s << ELEMENT_DELIMITER << e;
		});
	}
	s << VECTOR_RIGHT_BRACE;

	return s;
}

template<typename T>
llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const std::set<T>& set) {
	typedef typename std::set<T>::const_iterator ConstIter;

	using namespace::std;

	s << SET_LEFT_BRACE;
	if (!set.empty()) {
		ConstIter iter = set.begin();
		const T el = *iter++;
		s << el;
		for_each(iter, set.end(), [&s](const T& e){
			s << ELEMENT_DELIMITER << e;
		});
	}
	s << SET_RIGHT_BRACE;

	return s;
}

template<typename T>
llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const std::vector<T*>& vec) {
	typedef typename std::vector<T*>::const_iterator ConstIter;

	using namespace::std;

	s << VECTOR_LEFT_BRACE;
	if (!vec.empty()) {
		ConstIter iter = vec.begin();
		const T* el = *iter++;
		(el == NULL ? s << NULL_REPR : s << *el);
		for_each(iter, vec.end(), [&s](const T* e){
			s << ELEMENT_DELIMITER;
			(e == NULL ? s << NULL_REPR : s << *e);
		});
	}
	s << VECTOR_RIGHT_BRACE;

	return s;
}

template<typename T>
llvm::raw_ostream& operator <<(llvm::raw_ostream& s, const std::set<T*>& set) {
	typedef typename std::set<T*>::const_iterator ConstIter;

	using namespace::std;

	s << SET_LEFT_BRACE;
	if (!set.empty()) {
		ConstIter iter = set.begin();
		const T* el = *iter++;
		(el == NULL ? s << NULL_REPR : s << *el);
		for_each(iter, set.end(), [&s](const T* e){
			s << ELEMENT_DELIMITER;
			(e == NULL ? s << NULL_REPR : s << *e);
		});
	}
	s << SET_RIGHT_BRACE;

	return s;
}

} /* namespace std */

#endif /* UTIL_HPP_ */
