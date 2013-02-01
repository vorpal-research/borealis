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

#include "Logging/logger.hpp"
#include "Util/meta.hpp"
#include "Util/streams.hpp"

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util
//
////////////////////////////////////////////////////////////////////////////////

namespace borealis {
namespace util {

template<typename RetTy = void>
RetTy sayonara(std::string file, int line, std::string where, std::string reason) {
    using namespace borealis::logging;

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
    // XXX: remove?
    operator T&() { return get(); }

};

// copying reference
template<class T>
class copyref {
    std::unique_ptr<T> inner;

public:
    copyref(const copyref& ref) : inner(new T(*ref.inner)) {};
    // XXX: make explicit?
    copyref(const T& e) : inner(new T(e)) {};

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

#include "macros.h"

namespace impl {
template<class Tuple, class Callable, size_t ...N>
auto apply_packed_step_1(const Tuple& tp, Callable c, util::indexer<N...>)
QUICK_RETURN(c(std::get<N>(tp)...))
} // namespace impl

// apply a function taking N parameters to an N-tuple
template<class Callable, class ...Args>
auto apply_packed(Callable c, const std::tuple<Args...>& tp)
QUICK_RETURN(impl::apply_packed_step_1(tp, c, typename util::make_indexer<Args...>::type()))

#include "unmacros.h"

template<class T>
T* heap_copy(const T* val) {
    return new T(*val);
}

} // naemspace util
} // naemspace borealis

namespace llvm {
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

#endif /* UTIL_HPP_ */
