/*
 * util.hpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <llvm/Support/ManagedStatic.h>

#include <cstddef>

#include "lib/debugbreak/debugbreak.h"

#include "Logging/logger.hpp"
#include "Util/meta.hpp"
#include "Util/streams.hpp"

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util
//
////////////////////////////////////////////////////////////////////////////////

#include "macros.h"

namespace borealis {
namespace util {

template<typename RetTy = void>
RetTy sayonara(const std::string& file, int line, const std::string& where, const std::string& reason) {
    using namespace borealis::logging;

    debug_break();

    errs() << file << ":" << toString(line) << endl
            << "\t" << where << endl
            << "\t" << reason << endl;
    llvm::llvm_shutdown();
    std::exit(EXIT_FAILURE);
}

// java-style reference
template<class T>
class ref {
    T* ptr;

public:
    ref(const ref& that) = default;
    explicit ref(const T& v) : ptr(&v) {};

    const ref& operator=(const ref& that) = default;
    const ref& operator=(const T& v) {
        ptr = &v;
        return *this;
    }

    const T& get() const { return *ptr; }
    T& get() { return *ptr; }
    operator T&() { return get(); }

};

// copying reference
template<class T>
class copyref {
    std::unique_ptr<T> inner;

public:
    copyref(const copyref& ref) : inner(new T(*ref.inner)) {};
    explicit copyref(const T& e) : inner(new T(e)) {};

    const copyref& operator=(const copyref& ref) {
        inner.reset(new T(*ref.inner));
        return *this;
    }
    const copyref& operator=(const T& e) {
        inner.reset(new T(e));
        return *this;
    }

    T get() const { return *inner; }
    operator T() { return get(); }

};



namespace impl {
template<class Tuple, class Callable, size_t ...N>
auto apply_packed_step_1(const Tuple& tp, Callable c, util::indexer<N...>)
QUICK_RETURN(c(std::get<N>(tp)...))
} // namespace impl

// apply a function taking N parameters to an N-tuple
template<class Callable, class ...Args>
auto apply_packed(Callable c, const std::tuple<Args...>& tp)
QUICK_RETURN(impl::apply_packed_step_1(tp, c, typename util::make_indexer<Args...>::type()))



namespace impl {
template<class Tuple, size_t ...N>
auto cdr_tuple_step_1(const Tuple& tp, util::indexer<N...>)
QUICK_RETURN(std::make_tuple(std::get<N>(tp)...))
} // namespace impl

template<class ...Args>
auto cdr_tuple(const std::tuple<Args...>& tp)
QUICK_RETURN(impl::cdr_tuple_step_1(tp, typename util::make_cdr_indexer<Args...>::type()))



template<class T>
T* heap_copy(const T* val) {
    return new T(*val);
}

} // naemspace util
} // naemspace borealis

namespace llvm {
template<class To>
struct dyn_caster {
    template<class From>
    auto operator()(const From& from) QUICK_CONST_RETURN(dyn_cast<To>(from));
};

template<class T> struct simplify_type< std::shared_ptr<T> > {
    typedef T* SimpleType;
    static SimpleType getSimplifiedValue(const std::shared_ptr<T>& Val) {
        return Val.get();
    }
};

template<class T> struct simplify_type< const std::shared_ptr<T> > {
    typedef T* SimpleType;
    static SimpleType getSimplifiedValue(const std::shared_ptr<T>& Val) {
        return Val.get();
    }
};
} // namespace llvm

#include "unmacros.h"

#endif /* UTIL_HPP_ */
