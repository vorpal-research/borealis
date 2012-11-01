/*
 * streams.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */
#ifndef STREAMS_HPP_
#define STREAMS_HPP_

#include <set>
#include <vector>

#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>
#include <llvm/Value.h>

namespace borealis {
namespace util {

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
    s.changeColor(s.RED);
    s << v.val;
    s.resetColor();

    return s;
}

template<class T>
std::ostream& operator <<(std::ostream& s, const error_printer<T>& v) {
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



#endif /* STREAMS_HPP_ */
