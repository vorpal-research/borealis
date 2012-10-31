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

#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>
#include <llvm/Value.h>

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util
//
////////////////////////////////////////////////////////////////////////////////

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

template<class K, class V>
void removeFromMultimap(std::multimap<K,V>& map, const K& k, const V& v) {
    auto range = map.equal_range(k);

    for(auto it = range.first; it != range.second; ++it) {
        if(it->second == v) {
            map.erase(it);
            break;
        }
    }
}

template<class Container, class T>
bool contains(const Container& con, const T& t) {
	if (std::find(con.begin(), con.end(), t) != con.end()) return true;
	else return false;
}

namespace impl_ {
namespace {

template<class Tup, class Op, size_t N = std::tuple_size<Tup>::value>
struct tuple_for_each_helper {
    enum{ tsize = std::tuple_size<Tup>::value };
    inline static void apply(Tup& tup, Op op) {
        op(std::get<tsize-N>(tup));
        tuple_for_each_helper<Tup, Op, N-1>::apply(tup, op);
    }
};

template<class Tup, class Op>
struct tuple_for_each_helper<Tup, Op, 0> {
    inline static void apply(Tup&, Op) {};
};

} // namespace
} // namespace impl_

template<class Tup, class Op>
void tuple_for_each(Tup& tup, Op op) {
    typedef impl_::tuple_for_each_helper<Tup, Op> delegate;
    return delegate::apply(tup, op);
}

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util::enums
//
////////////////////////////////////////////////////////////////////////////////

namespace enums {

template<typename Enum>
typename std::underlying_type<Enum>::type asInteger(const Enum& e) {
    return static_cast<typename std::underlying_type<Enum>::type>(e);
}

} // namespace enums

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util
//
////////////////////////////////////////////////////////////////////////////////

template<class T>
struct is_using_llvm_output {
    enum { value =
            std::is_base_of<llvm::Value, T>::value ||
            std::is_base_of<llvm::Type, T>::value ||
            std::is_base_of<llvm::StringRef, T> :: value ||
            std::is_base_of<llvm::Twine, T>::value ||
            std::is_base_of<llvm::Module, T>::value ||
            std::is_base_of<llvm::Pass, T>::value
    };
};

template<class T>
struct UseLLVMOstreams {
	enum { value = is_using_llvm_output<T>::value };
};

template<class T,
bool UseLLVMOstream = false>
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

template<typename RetTy = void>
RetTy sayonara(std::string file, int line, std::string reason) {
   llvm::errs() << file << ":" << toString(line) << " "
           << reason << streams::endl;
   std::exit(EXIT_FAILURE);
   return *((RetTy*)nullptr);
}

// generalized hash for enum classes
template<class Enum>
struct enum_hash {
    typedef typename std::underlying_type<Enum>::type raw;
    std::hash<raw> delegate;

    inline size_t operator()( Enum e ) const {
      return delegate(static_cast<raw>(e));
    }
};

inline static size_t hash_combiner(size_t left, size_t right) // replaceable
{ return left^right; }

template<int index, class...types>
struct hash_impl {
    size_t operator()(size_t a, const std::tuple<types...>& t) const {
        typedef typename std::tuple_element<index, std::tuple<types...>>::type nexttype;
        hash_impl<index-1, types...> next;
        size_t b = std::hash<nexttype>()(std::get<index>(t));
        return next(hash_combiner(a, b), t);
    }
};

template<class...types>
struct hash_impl<0, types...> {
    size_t operator()(size_t a, const std::tuple<types...>& t) const {
        typedef typename std::tuple_element<0, std::tuple<types...>>::type nexttype;
        size_t b = std::hash<nexttype>()(std::get<0>(t));
        return hash_combiner(a, b);
    }
};

template<class T, class ...List>
struct get_index_of_T_in;

template<class T, class ...Tail>
struct get_index_of_T_in<T, T, Tail...> {
    enum{ value = 0 };
};

// Head != T
template<class T, class Head, class ...Tail>
struct get_index_of_T_in<T, Head, Tail...> {
    enum{ value = get_index_of_T_in<T, Tail...>::value + 1 };
};

template<class T, class ...List>
struct is_T_in;

template<class T, class ...Tail>
struct is_T_in<T,T,Tail...> : std::true_type {};

template<class T, class H, class ...Tail>
struct is_T_in<T,H,Tail...> : is_T_in<T, Tail...>{};

template<class T>
struct is_T_in<T> : std::false_type {};

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util::streams
//
////////////////////////////////////////////////////////////////////////////////

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

template<class T>
std::ostream& operator <<(std::ostream& s, const error_printer<T>& v) {
    using namespace::std;

    s << "!";
    s << v.val;
    s << "!";

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

} // namespace streams
} // namespace util
} // namespace borealis

////////////////////////////////////////////////////////////////////////////////
//
// std
//
////////////////////////////////////////////////////////////////////////////////

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

template<class...types>
struct hash<std::tuple<types...>> {
    size_t operator()(const std::tuple<types...>& t) const {
        const size_t begin = std::tuple_size<std::tuple<types...>>::value-1;
        return borealis::util::hash_impl<begin, types...>()(1, t); //1 should be some largervalue
    }
};

template<class T, class U>
struct hash<std::pair<T, U>> {
    size_t operator()(const std::pair<T, U >& t) const {
        const size_t begin = 1;
        return borealis::util::hash_impl<begin, T, U>()(1, t); //1 should be some largervalue
    }
};

} // namespace std

////////////////////////////////////////////////////////////////////////////////
//
// llvm
//
////////////////////////////////////////////////////////////////////////////////

namespace llvm {
    template<class T, class Check = typename std::enable_if<
            borealis::util::is_using_llvm_output<T>::value
    >::type >
    std::ostream& operator <<(std::ostream& ost, const T& llvm_val) {

        std::string buf;
        llvm::raw_string_ostream ostt(buf);
        ostt << llvm_val;
        return std::operator<<(ost, ostt.str());
    }
} // namespace llvm

#endif /* UTIL_HPP_ */
