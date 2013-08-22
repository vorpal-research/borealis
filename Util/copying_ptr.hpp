/*
 * copying_ptr.hpp
 *
 *  Created on: Aug 22, 2013
 *      Author: belyaev
 */

#ifndef COPYING_PTR_HPP_
#define COPYING_PTR_HPP_

#include <memory>

#include "Util/type_traits.hpp"

/*******************************************************************************
 *
 * a rather meaningless but useful smart ptr that just copies the object
 * along with itself
 * does not work for arrays 'cos nobody knows how to copy them
 *
 ******************************************************************************/
namespace borealis {
namespace util{

template<class T>
struct default_copy {
    T* operator()(const T* val) const {
        if(!val) return nullptr;
        return new T(*val);
    }
};

template<
    class T,
    class Deleter = std::default_delete<T>,
    class Copier = default_copy<T> >
class copying_ptr {
    typedef std::unique_ptr<T, Deleter> holder_t;
    holder_t inner;

    typedef copying_ptr self;
public:
    typedef typename holder_t::pointer pointer;
    typedef typename holder_t::element_type element_type;
    typedef typename holder_t::deleter_type deleter_type;
    typedef Copier copier_type;

    copying_ptr(std::nullptr_t) noexcept: inner{ nullptr } {};
    copying_ptr(pointer hld) noexcept: inner{ hld } {};
    copying_ptr(const holder_t& hld) noexcept: inner{ hld } {};
    copying_ptr(const self& that): inner{ Copier()(that.inner.get()) } {}
    copying_ptr(self&&) = default;
    copying_ptr() = default;

    void swap(self& other) noexcept {
        std::swap(inner, other.inner);
    }

    copying_ptr& operator=(const self& that) {
        self tmp = that;
        this->swap(tmp);
        return *this;
    }
    copying_ptr& operator=(self&&) = default;
    copying_ptr& operator=(std::nullptr_t) noexcept{
        inner.reset(nullptr);
        return *this;
    }

    pointer release() noexcept{
        return inner.release();
    }

    pointer get() const noexcept{
        return inner.get();
    }

    Deleter get_deleter() const noexcept{
        return inner.get_deleter();
    }

    add_lvalue_reference_t<T> operator*() const {
        return *inner;
    }

    holder_t operator->() noexcept{
        return inner;
    }

    operator bool() const noexcept{
        return !!inner;
    }

    bool operator!() const noexcept {
        return !inner;
    }

    template<class T2, class D2, class C2>
    bool equals(const copying_ptr<T2, D2, C2>& that) {
        if(!that) return !this->inner;
        return *inner == *that.inner;
    }
};

template<class T1, class D1, class C1, class T2, class D2, class C2>
bool operator==(const copying_ptr<T1, D1, C1>& x, const copying_ptr<T2, D2, C2>& y) {
    return x.equals(y);
}

template<class T1, class D1, class C1>
bool operator==(const copying_ptr<T1, D1, C1>& x, std::nullptr_t) {
    return !x;
}

template<class T1, class D1, class C1>
bool operator==(std::nullptr_t, const copying_ptr<T1, D1, C1>& x) {
    return !x;
}

template<class T1, class D1, class C1, class T2, class D2, class C2>
bool operator!=(const copying_ptr<T1, D1, C1>& x, const copying_ptr<T2, D2, C2>& y) {
    return !x.equals(y);
}

template<class T1, class D1, class C1>
bool operator!=(const copying_ptr<T1, D1, C1>& x, std::nullptr_t) {
    return !!x;
}

template<class T1, class D1, class C1>
bool operator!=(std::nullptr_t, const copying_ptr<T1, D1, C1>& x) {
    return !!x;
}

} // namespace util
} // namespace borealis

namespace std {

template<class T, class D, class C>
void swap(const borealis::util::copying_ptr<T, D, C>& ptr1,
        const borealis::util::copying_ptr<T, D, C>& ptr2) noexcept {
    return ptr1.swap(ptr2);
}

template<class T, class D, class C>
struct hash<borealis::util::copying_ptr<T, D, C>> {
    size_t operator()(const borealis::util::copying_ptr<T, D, C>& ptr) const noexcept{
        return std::hash<T*>()(ptr.get());
    }
};

}

#endif /* COPYING_PTR_HPP_ */
