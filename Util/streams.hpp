/*
 * streams.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef STREAMS_HPP_
#define STREAMS_HPP_

#include <clang/AST/Decl.h>
#include <llvm/Analysis/AliasSetTracker.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Pass.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "Util/collections.hpp"
#include "Util/option.hpp"
#include "Util/sayonara.hpp"
#include "Util/type_traits.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

struct byte_output {
    const char* bytes;
    size_t length;
};

template<class Streamer>
Streamer& operator<<(Streamer& str, const byte_output& bytes) {
    str.write(bytes.bytes, bytes.length);
    return str;
}


template<class T>
struct is_using_llvm_output {
    enum { value =
            std::is_base_of<clang::Decl, T>::value ||
            std::is_base_of<llvm::Value, T>::value ||
            std::is_base_of<llvm::Type, T>::value ||
            std::is_base_of<const llvm::StringRef, T>::value ||
            std::is_base_of<llvm::SmallVectorBase, T>::value ||
            std::is_base_of<llvm::Twine, T>::value ||
            std::is_base_of<llvm::Module, T>::value ||
            std::is_base_of<llvm::Pass, T>::value ||
            std::is_base_of<llvm::APInt, T>::value ||
            std::is_base_of<llvm::SCEV, T>::value ||
            std::is_base_of<llvm::AliasSetTracker, T>::value ||
            std::is_base_of<llvm::AliasSet, T>::value
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

template<class T, class SFINAE = void>
struct Destringifier {
    static option<T> fromString(const std::string& str) {
        std::istringstream is(str);
        T val;
        is >> val;
        if (!is) return nothing();
        return just(val);
    }
};

template<>
struct Destringifier<std::string> {
    static option<std::string> fromString(const std::string& str) {
        return just(str);
    }
};

template<>
struct Destringifier<bool> {
    static option<bool> fromString(const std::string& str) {
        return ("true" == str) ? just(true) :
               ("false" == str) ? just(false) :
               nothing();
    }
};

template<class T>
struct Destringifier<T, GUARD(std::is_integral<T>::value)> {
    static option<T> fromString(const std::string& str) {
        auto mod = std::dec;
        auto shift = 0;
        if (str.substr(0, 2) == "0x") {
            mod = std::hex;
            shift = 2;
        }
        // seems to be replaced by sexpr[0] == '0', but substr is safer
        else if (str.substr(0, 1) == "0" && str != "0") {
            mod = std::oct;
            shift = 1;
        }
        // we rely on shift being 0 if the string is empty
        std::istringstream is(str.c_str() + shift);
        T val;
        is >> mod >> val;
        if (!is) return nothing();
        return just(val);
    }
};

template<class T>
inline option<T> fromString(const std::string& str) {
    return Destringifier<T>::fromString(str);
}

template<class T, class V>
inline option<T> stringCast(V&& obj) {
    return Destringifier<T>::fromString(
        util::toString(std::forward<V>(obj))
    );
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
    return s << v.val;
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

template<class Streamer>
class llvm_stream_wrapper : public llvm::raw_ostream {
    Streamer* str;
public:
    llvm_stream_wrapper(Streamer& str): str(&str) {};

    virtual void write_impl(const char* Ptr, size_t Size) {
        (*str) << byte_output{ Ptr, Size };
    }

    /// current_pos - Return the current position within the stream, not
    /// counting the bytes currently in the buffer.
    virtual uint64_t current_pos() const {
        // FIXME: std::ostream::tellp() should work in most cases
        //        BUT! log4cpp::CategoryStream does not have that
        return static_cast<uint64_t>(-1);
    }
};

template<class Container>
struct separated_printer {
    const Container& con;
    std::string sep;
};

template<class Con>
separated_printer<Con> delimited(const Con& con, const std::string& sep) {
    return separated_printer<Con>{ con, sep };
}

template<class Con>
separated_printer<Con> delimited(const Con& con) {
    return delimited(con, ", ");
}

template<typename Con, typename Streamer>
Streamer& operator<<(Streamer& s, const separated_printer<Con>& pp) {
    using borealis::util::head;
    using borealis::util::tail;

    if (std::begin(pp.con) != std::end(pp.con)) {
        s << head(pp.con);
        for (const auto& e : tail(pp.con)) {
            s << pp.sep << e;
        }
    }

    return s;
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




template<typename T, typename U>
std::ostream& operator<<(std::ostream& s, const std::pair<T, U>& pp) {
    using namespace std::impl_;

    s << TUPLE_LEFT_BRACE
      << pp.first
      << ELEMENT_DELIMITER
      << pp.second
      << TUPLE_RIGHT_BRACE;

    return s;
}



template<typename Elem, typename SFINAE = void>
struct elemPrettyPrinter {
    template<class Streamer>
    static Streamer& doit(Streamer& s, const Elem& e) {
        // this is generally fucked up...
        return static_cast<Streamer&>(s << e);
    }
};

template<typename Elem>
struct elemPrettyPrinter<Elem, GUARD(std::is_pointer<Elem>::value)> {
    template<class Streamer>
    static Streamer& doit(Streamer& s, const Elem& e) {
        // this is generally fucked up...
        return static_cast<Streamer&>(e == nullptr ? s << impl_::NULL_REPR : s << *e);
    }
};

template<typename Elem>
struct elemPrettyPrinter<Elem, GUARD(borealis::util::is_shared_ptr<Elem>::value)> {
    template<class Streamer>
    static Streamer& doit(Streamer& s, const Elem& e) {
        // this is generally fucked up...
        return static_cast<Streamer&>(e == nullptr ? s << impl_::NULL_REPR : s << e);
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
        using borealis::util::head;
        using borealis::util::tail;

        s << LB;
        if (!con.empty()) {
            epp::doit(s, head(con));
            for (const auto& e : tail(con)) {
                s << ELEMENT_DELIMITER;
                epp::doit(s, e);
            }
        }
        s << RB;

        return s;
    }
};

template<typename T, typename A, typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& s, const std::vector<T, A>& vec) {
    typedef std::vector<T, A> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::VECTOR_LEFT_BRACE, impl_::VECTOR_RIGHT_BRACE>(s, vec);
}

template<typename T, typename Compare, typename A, typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& s, const std::set<T, Compare, A>& set) {
    typedef std::set<T, Compare, A> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::SET_LEFT_BRACE, impl_::SET_RIGHT_BRACE>(s, set);
}

template<typename T, typename Hash, typename Pred, typename A, typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& s, const std::unordered_set<T, Hash, Pred, A>& set) {
    typedef std::unordered_set<T, Hash, Pred, A> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::SET_LEFT_BRACE, impl_::SET_RIGHT_BRACE>(s, set);
}

template<typename K, typename V, typename Compare, typename A, typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& s, const std::map<K, V, Compare, A>& map) {
    typedef std::map<K, V, Compare, A> Container;
    return containerPrettyPrinter< Container >::template doit<impl_::SET_LEFT_BRACE, impl_::SET_RIGHT_BRACE>(s, map);
}

template<typename K, typename V, typename Hash, typename Pred, typename A, typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>&  operator<<(std::basic_ostream<CharT, Traits>& s, const std::unordered_map<K, V, Hash, Pred, A>& map) {
    typedef std::unordered_map<K, V, Hash, Pred, A> Container;
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
