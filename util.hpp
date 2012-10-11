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

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Casting.h>

namespace borealis {



namespace util {

// stupid straightforward collection view
//  - cannot be used after any operation with begin or end
//  - should be used in-place only, like this:
//      int arr[] = { 0,1,2,3,4,5,6,7,8,9,10 };
//      for_each(CollectionView<int*>(arr, arr+10), [](const Elem& el){ ... });
template<class ContainerIter>
class CollectionView {
	ContainerIter begin_;
	ContainerIter end_;
public:
	CollectionView(ContainerIter begin, ContainerIter end): begin_(begin), end_(end) {
	}
	CollectionView(std::pair<ContainerIter, ContainerIter> iters): begin_(iters.first), end_(iters.second) {
	}

	ContainerIter begin() const { return begin_; }
	ContainerIter end() const { return end_; }
	bool empty() const { return begin_ == end_; }
};

template<class Container>
inline auto head(const Container& con) -> decltype(*con.begin()) {
	return *con.begin();
}

template<class Iter>
inline auto view(Iter b, Iter e) -> CollectionView<Iter> {
	return CollectionView<Iter>(b, e);
}

template<class Iter>
inline auto view(const std::pair<Iter,Iter>& is) -> CollectionView<Iter> {
	return CollectionView<Iter>(is);
}

template<class Container>
inline auto tail(const Container& con) -> CollectionView<decltype(con.begin())> {
	return view(++con.begin(), con.end());
}



template<class Container, class Callable>
inline void for_each(const Container& con, const Callable cl) {
	std::for_each(con.begin(), con.end(), cl);
}

template<class Container, class Callable>
inline void for_each(Container& con, const Callable cl) {
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
struct UseLLVMOstreams {
	enum{ value = false };
};

template<class T, bool UseLLVMOstream = false>
struct Stringifier;

template<class T>
struct Stringifier<T, false> {
	static std::string toString(const T& t) {
		std::ostringstream oss;
		oss << t;
		return oss.str();
	}
};

template<class T>
struct Stringifier<T, true> {
	static std::string toString(const T& t) {
		std::string buf;
		llvm::raw_string_ostream oss(buf);
		oss << t;
		return oss.str();
	}
};

// special cases
template<>
struct Stringifier<bool> {
	static std::string toString(bool t) {
		return t?"true":"false";
	}
};

template<>
struct Stringifier<std::string> {
	static std::string toString(const std::string& t) {
		return t;
	}
};

template<>
struct Stringifier<const char*> {
	static std::string toString(const char* t) {
		return t;
	}
};

template<class T>
inline std::string toString(const T& t) {
	return Stringifier<T, UseLLVMOstreams<T>::value>::toString(t);
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

template<class Func>
std::string with_stream(Func f) {
	std::ostringstream ost;
	f(ost);
	return ost.str();
}

template<class Func>
std::string with_llvm_stream(Func f) {
	std::string buf;
	llvm::raw_string_ostream ost(buf);
	f(ost);
	return ost.str();
}

} /* namespace streams */
} /* namespace util */
} /* namespace borealis */

namespace std {

// Custom output routines and such...
const auto VECTOR_LEFT_BRACE = "[";
const auto VECTOR_RIGHT_BRACE = "]";
const auto SET_LEFT_BRACE = "(";
const auto SET_RIGHT_BRACE = ")";
const auto ELEMENT_DELIMITER = ", ";
const auto NULL_REPR = "<NULL>";

template<typename T, typename Streamer>
Streamer& operator <<(Streamer& s, const std::vector<T>& vec) {
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

template<typename T, typename Streamer>
Streamer& operator <<(Streamer& s, const std::set<T>& set) {
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

template<typename T, typename Streamer>
Streamer& operator <<(Streamer& s, const std::vector<T*>& vec) {
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

template<typename T, typename Streamer>
Streamer& operator <<(Streamer& s, const std::set<T*>& set) {
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
