/*
 * streams.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef STREAMS_HPP_
#define STREAMS_HPP_

#include <clang/AST/Decl.h>
#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>
#include <llvm/Value.h>

#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <Util/collections.hpp>

#include "Util/macros.h"

namespace borealis {
namespace util {

template<class T>
struct is_using_llvm_output {
    enum { value =
            std::is_base_of<clang::Decl, T>::value ||
            std::is_base_of<llvm::Value, T>::value ||
            std::is_base_of<llvm::Type, T>::value ||
            std::is_base_of<llvm::StringRef, T>::value ||
            std::is_base_of<llvm::Twine, T>::value ||
            std::is_base_of<llvm::Module, T>::value ||
            std::is_base_of<llvm::Pass, T>::value
    };
};

template<class T>
struct UseLLVMOstreams {
    enum { value = is_using_llvm_output<T>::value };
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
        return t ? "true" : "false";
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
    error_printer(const T& v) : val(v) {}
};

template<class T>
std::ostream& operator<<(std::ostream& s, const error_printer<T>& v) {
    return s << "!" << v.val << "!";
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

template<class T>
std::ostream& output_using_llvm(std::ostream& ost, const T& val) {
    std::string buf;
    llvm::raw_string_ostream ostt(buf);
    ostt << val;
    return ost << ostt.str();
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

namespace impl_ {
// Custom output stuff and such...
const auto VECTOR_LEFT_BRACE = '[';
const auto VECTOR_RIGHT_BRACE = ']';
const auto SET_LEFT_BRACE = '(';
const auto SET_RIGHT_BRACE = ')';
const auto ELEMENT_DELIMITER = ", ";
const auto TUPLE_LEFT_BRACE = '{';
const auto TUPLE_RIGHT_BRACE = '}';
const auto NULL_REPR = "<NULL>";
} // namespace impl_



template<typename T, typename U, typename Streamer>
Streamer& operator<<(Streamer& s, const std::pair<T, U>& pp) {
    using namespace std::impl_;

    s << TUPLE_LEFT_BRACE
      << pp.first
      << ELEMENT_DELIMITER
      << pp.second
      << TUPLE_RIGHT_BRACE;

    return s;
}



template<typename Elem, typename SFINAE = void>
struct elemPrettyPrinter;

template< typename Elem >
struct elemPrettyPrinter<Elem, GUARD(std::is_pointer<Elem>::value)> {
    template<class Streamer>
    static Streamer& doit(Streamer& s, const Elem& e) {
        // this is generally fucked up...
        return static_cast<Streamer&>(e == nullptr ? s << impl_::NULL_REPR : s << *e);
    }
};

template< typename Elem >
struct elemPrettyPrinter<Elem, GUARD(!std::is_pointer<Elem>::value)> {
    template<class Streamer>
    static Streamer& doit(Streamer& s, const Elem& e) {
        // this is generally fucked up...
        return static_cast<Streamer&>(s << e);
    }
};

template<>
struct elemPrettyPrinter<const char*> {
    template<class Streamer>
    static Streamer& doit(Streamer& s, const char* e) {
        // this is generally fucked up...
        return static_cast<Streamer&>(e == nullptr ? s << impl_::NULL_REPR : s << e);
    }
};



template< class Container >
struct containerPrettyPrinter {
    template<char LB, char RB, class Streamer>
    static Streamer& doit(Streamer& s, const Container& con) {
        typedef typename Container::const_iterator ConstIter;
        typedef typename Container::value_type Elem;
        typedef elemPrettyPrinter<Elem> epp;

        using namespace std::impl_;
        using borealis::util::view;

        s << LB;
        if (!con.empty()) {
            ConstIter iter = con.begin();
            const Elem& el = *iter++;
            epp::doit(s, el);
            for (const auto& e : view(iter, con.end())) {
                s << ELEMENT_DELIMITER;
                epp::doit(s, e);
            }
        }
        s << RB;

        return s;
    }
};

template<typename T, typename Streamer>
Streamer& operator<<(Streamer& s, const std::vector<T>& vec) {
    typedef std::vector<T> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::VECTOR_LEFT_BRACE, impl_::VECTOR_RIGHT_BRACE>(s, vec);
}

template<typename T, typename Streamer>
Streamer& operator<<(Streamer& s, const std::set<T>& set) {
    typedef std::set<T> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::SET_LEFT_BRACE, impl_::SET_RIGHT_BRACE>(s, set);
}

template<typename T, typename Streamer>
Streamer& operator<<(Streamer& s, const std::unordered_set<T>& set) {
    typedef std::unordered_set<T> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::SET_LEFT_BRACE, impl_::SET_RIGHT_BRACE>(s, set);
}

template<typename K, typename V, typename Streamer>
Streamer& operator<<(Streamer& s, const std::unordered_map<K, V>& map) {
    typedef std::unordered_map<K, V> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::SET_LEFT_BRACE, impl_::SET_RIGHT_BRACE>(s, map);
}

} // namespace std

////////////////////////////////////////////////////////////////////////////////
//
// llvm
//
////////////////////////////////////////////////////////////////////////////////

namespace llvm {

inline raw_ostream& operator<<(raw_ostream& ost, const Pass& pass) {
    pass.print(ost, nullptr);
    return ost;
}

template<class T, class Check = GUARD(borealis::util::is_using_llvm_output<T>::value)>
std::ostream& operator<<(std::ostream& ost, const T& llvm_val) {
    return borealis::util::streams::output_using_llvm(ost, llvm_val);
}

} // namespace llvm

////////////////////////////////////////////////////////////////////////////////
//
// clang
//
////////////////////////////////////////////////////////////////////////////////

namespace clang {

inline raw_ostream& operator <<(raw_ostream& ost, const Decl& decl) {
    decl.print(ost, 0, true);
    return ost;
}

template<class T, class Check = GUARD(borealis::util::is_using_llvm_output<T>::value)>
std::ostream& operator<<(std::ostream& ost, const T& llvm_val) {
    return borealis::util::streams::output_using_llvm(ost, llvm_val);
}

} // namespace clang

#include "Util/unmacros.h"

#endif /* STREAMS_HPP_ */
