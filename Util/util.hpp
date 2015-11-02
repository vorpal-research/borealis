/*
 * util.hpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_HPP_
#define UTIL_HPP_

#include "Logging/logger.hpp"
#include "Util/meta.hpp"
#include "Util/streams.hpp"

#include "Util/macros.h"

////////////////////////////////////////////////////////////////////////////////
//
// borealis::util
//
////////////////////////////////////////////////////////////////////////////////

namespace borealis {
namespace util {

template<class ...T>
inline void use(T&&...) noexcept {}

template<class ...Args>
std::vector<typename std::common_type<Args...>::type> make_vector(Args&& ...args) {
    return std::vector<typename std::common_type<Args...>::type>{ std::forward<Args>(args)... };
}

template<class T>
std::shared_ptr<T> copy_or_share(const T& value) {
    return std::make_shared<T>(value);
}

template<class T>
std::shared_ptr<T> copy_or_share(std::enable_shared_from_this<T>& value) {
    return value.shared_from_this();
}

template<class T>
std::shared_ptr<const T> copy_or_share(const std::enable_shared_from_this<T>& value) {
    return value.shared_from_this();
}

// java-style reference
template<class T>
class ref {
    T* ptr;

public:
    ref() : ptr(nullptr) {};
    ref(const ref& that) = default;
    explicit ref(const T& v) : ptr(&v) {};
    explicit ref(T& v) : ptr(&v) {};

    ref& operator=(const ref& that) = default;
    ref& operator=(T& v) noexcept {
        ptr = &v;
        return *this;
    }

    const T& get() const { return *ptr; }
    T& get() { return *ptr; }
    const T* getPtr() const noexcept { return ptr; }
    T* getPtr() noexcept { return ptr; }

    operator T&() { return get(); }

};

// copying reference
template<class T>
class copyref {
    std::unique_ptr<T> inner;

public:
    copyref() : inner{nullptr} {};
    copyref(const copyref& ref) : inner{ new T{*ref.inner} } {};
    explicit copyref(const T& e) : inner{ new T{e} } {};

    const copyref& operator=(const copyref& ref) {
        inner.reset( new T{*ref.inner} );
        return *this;
    }
    const copyref& operator=(const T& e) {
        inner.reset( new T{e} );
        return *this;
    }

    const T& get() const { return *inner; }
    operator const T&() const { return get(); }
};





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
    return new T{ *val };
}

template<class T>
std::unique_ptr<T> uniq(T* val) {
    return std::unique_ptr<T>(val);
}

template<class T, class D>
std::unique_ptr<T, D> uniq(T* val, D deleter) {
    return std::unique_ptr<T, D>(val, deleter);
}

// FIXME: This or std::make_unique(...) ???
template<class T, class ...Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>{ new T{ std::forward<Args>(args)... } };
}

namespace impl_ {
template<class T> struct friend_delegate;
} /* namespace impl_ */

template<class T, class Delegate = void>
class enable_special_make_shared {
    T value;
    friend struct impl_::friend_delegate<Delegate>;
public:
    template<class ...Args>
    enable_special_make_shared(Args&&... args) : value(std::forward<Args>(args)...){}
};

namespace impl_ {

template<class T>
struct friend_delegate {
    friend_delegate() = delete;
    friend_delegate(const T&){}
    friend_delegate(T&&){}

    template<class Wrapped>
    inline std::shared_ptr<Wrapped> unwrap(std::shared_ptr<enable_special_make_shared<Wrapped, T>>& wrapper) {
        return std::shared_ptr<Wrapped>(wrapper, &wrapper->value);
    }
};

template<>
struct friend_delegate<void> {
    template<class Wrapped>
    inline std::shared_ptr<Wrapped> unwrap(std::shared_ptr<enable_special_make_shared<Wrapped>>& wrapper) {
        return std::shared_ptr<Wrapped>(wrapper, &wrapper->value);
    }
};

} /* namespace impl_ */

template<class T, class ...Args>
inline std::shared_ptr<T> make_shared(Args&&... args) {
    auto magic = std::make_shared<enable_special_make_shared<T>>(std::forward<Args>(args)...);
    return impl_::friend_delegate<void>().unwrap(magic);
}

template<class T, class Factory, class ...Args>
inline std::shared_ptr<T> make_shared_restricted(const Factory& f, Args&&... args) {
    auto magic = std::make_shared<enable_special_make_shared<T, Factory>>(std::forward<Args>(args)...);
    return impl_::friend_delegate<Factory>(f).unwrap(magic);
}

namespace impl {
template<char Sep>
std::ostringstream& concat_(std::ostringstream& s) {
    return s;
}
template<char Sep, class H, class ...T>
std::ostringstream& concat_(std::ostringstream& s, const H& h, const T&... t) {
    // this is generally fucked up...
    return concat_<Sep, T...>(static_cast<std::ostringstream&>(s << Sep << h), t...);
}
template<char Sep, class H, class ...T>
std::ostringstream& concat(std::ostringstream& s, const H& h, const T&... t) {
    // this is generally fucked up...
    return concat_<Sep, T...>(static_cast<std::ostringstream&>(s << h), t...);
}
} // namespace impl

template<char Sep = ' ', class ...Args>
std::string join(const Args&... args) {
    std::ostringstream s;
    impl::concat<Sep, Args...>(s, args...);
    return s.str();
}

} // naemspace util
} // naemspace borealis

namespace llvm {
template<class To>
struct dyn_caster {
    template<class From>
    auto operator()(From&& from) QUICK_CONST_RETURN(dyn_cast<To>(std::forward<From>(from)));
};

template<class To>
struct caster {
    template<class From>
    auto operator()(From&& from) QUICK_CONST_RETURN(cast<To>(std::forward<From>(from)));
};

template<class To>
struct isaer {
    template<class From>
    auto operator()(From&& from) QUICK_CONST_RETURN(isa<To>(std::forward<From>(from)));
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

#include "Util/unmacros.h"

#endif /* UTIL_HPP_ */
