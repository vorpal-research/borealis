#ifndef IRF_PTR_HPP
#define IRF_PTR_HPP

#include <memory>
#include <cstddef>

#include <llvm/Support/Casting.h>

#include "Util/generate_macros.h"

namespace borealis {
namespace util {

template<class T>
class irf_ptr;

template<class T, class Deleter>
class irfd_ptr;

class irf_base_base {
    mutable size_t counter = 0;

public:
    void inc() const { ++counter; }
    void dec() const { ++counter; }
    bool empty() const { return counter == 0; }

    ~irf_base_base() {}
};

template<class Deleter>
class irfd_base_base {
    mutable size_t counter = 0;
    mutable Deleter deleter_;

public:
    irfd_base_base(Deleter d): deleter_(d) {}
    irfd_base_base() = default;

    void inc() const { ++counter; }
    void dec() const { ++counter; }
    bool empty() const { return counter == 0; }

    Deleter& deleter() const { return deleter_; }

    ~irfd_base_base() {}
};

template<class T>
class irf_base: public irf_base_base {
public:
    irf_ptr<T> shared_from_this() const;
};

template<class T, class Deleter>
class irfd_base: public irfd_base_base<Deleter> {
public:
    irfd_base(Deleter d): irfd_base_base<Deleter>(d) {}
    irfd_base() = default;

    irfd_ptr<T, Deleter> shared_from_this() const;
};

template<class T>
class irf_ptr {
    const irf_base_base* data;

    void acquire() {
        if(data) data->inc();
    }

public:
    void reset() {
        if(data) {
            data->dec();
            if(data->empty()) {
                std::default_delete<T>{}(static_cast<T*>(data));
            }
        }
        data = nullptr;
    }

    friend void swap(irf_ptr& l, irf_ptr& r) {
        using std::swap;
        swap(l.data, r.data);
    }

    irf_ptr(): data(nullptr) {}
    irf_ptr(std::nullptr_t): data(nullptr) {}
    explicit irf_ptr(T* data): data(data) { acquire(); }
    irf_ptr(const irf_ptr& that): data(that.data) { acquire(); }
    irf_ptr(irf_ptr&& that): irf_ptr() { swap(*this, that); }
    template<class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    irf_ptr(const irf_ptr<U>& that): irf_ptr(that.get()) {}
    template<class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    explicit irf_ptr(std::unique_ptr<U>&& that): irf_ptr(that.release()) {}
    irf_ptr& operator=(std::nullptr_t) {
        reset();
        return *this;
    }
    irf_ptr& operator=(const irf_ptr& that) {
        irf_ptr copy(that);
        swap(*this, copy);
        return *this;
    }
    irf_ptr& operator=(irf_ptr&& that) {
        irf_ptr copy(std::move(that));
        swap(*this, copy);
        return *this;
    }
    ~irf_ptr() { reset(); }

    T* get() const { return static_cast<T*>(data); }
    explicit operator bool() const { return get() != nullptr; }
    bool operator not() const { return get() == nullptr; }

    std::add_lvalue_reference_t<T> operator*() const {
        return *get();
    }

    T* operator->() const {
        return get();
    }

    friend bool operator==(std::nullptr_t, irf_ptr rhv) { return not rhv; }
    friend bool operator==(irf_ptr lhv, std::nullptr_t) { return not lhv; }
    friend bool operator!=(std::nullptr_t, irf_ptr rhv) { return not not rhv; }
    friend bool operator!=(irf_ptr lhv, std::nullptr_t) { return not not lhv; }

    friend bool operator==(irf_ptr lhv, irf_ptr rhv) { return lhv.get() == rhv.get(); }
    friend bool operator<(irf_ptr lhv, irf_ptr rhv) { return lhv.get() < rhv.get(); }
    GENERATE_AUX_COMPARISONS(irf_ptr);

};

template<class T, class Deleter>
class irfd_ptr {
    const irfd_base_base<Deleter>* data;

    void acquire() {
        if(data) data->inc();
    }

    void do_delete() {
        if(data) {
            auto deleter = std::move(data->deleter());
            deleter(static_cast<T*>(data));
        }
    }

public:
    void reset() {
        if(data) {
            data->dec();
            if(data->empty()) {
                do_delete();
            }
        }
        data = nullptr;
    }

    friend void swap(irfd_ptr& l, irfd_ptr& r) {
        using std::swap;
        swap(l.data, r.data);
    }

    irfd_ptr(): data(nullptr) {}
    irfd_ptr(std::nullptr_t): data(nullptr) {}
    explicit irfd_ptr(T* data): data(data) { acquire(); }
    irfd_ptr(const irfd_ptr& that): data(that.data) { acquire(); }
    irfd_ptr(irfd_ptr&& that): irfd_ptr() { swap(*this, that); }
    template<class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    irfd_ptr(const irfd_ptr<U, Deleter>& that): irfd_ptr(that.get()) {}
    template<class U, class = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    explicit irfd_ptr(std::unique_ptr<U>&& that): irfd_ptr(that.release()) {}
    irfd_ptr& operator=(std::nullptr_t) {
        reset();
        return *this;
    }
    irfd_ptr& operator=(const irfd_ptr& that) {
        irfd_ptr copy(that);
        swap(*this, copy);
        return *this;
    }
    irfd_ptr& operator=(irfd_ptr&& that) {
        irfd_ptr copy(std::move(that));
        swap(*this, copy);
        return *this;
    }
    ~irfd_ptr() { reset(); }

    T* get() const { return static_cast<T*>(data); }
    explicit operator bool() const { return get() != nullptr; }
    bool operator not() const { return get() == nullptr; }

    std::add_lvalue_reference_t<T> operator*() const {
        return *get();
    }

    T* operator->() const {
        return get();
    }

    friend bool operator==(std::nullptr_t, irfd_ptr rhv) { return not rhv; }
    friend bool operator==(irfd_ptr lhv, std::nullptr_t) { return not lhv; }
    friend bool operator!=(std::nullptr_t, irfd_ptr rhv) { return not not rhv; }
    friend bool operator!=(irfd_ptr lhv, std::nullptr_t) { return not not lhv; }

    friend bool operator==(irfd_ptr lhv, irfd_ptr rhv) { return lhv.get() == rhv.get(); }
    friend bool operator<(irfd_ptr lhv, irfd_ptr rhv) { return lhv.get() < rhv.get(); }
    GENERATE_AUX_COMPARISONS(irfd_ptr);

};

template<class T, class ...Args>
irf_ptr<T> make_irf(Args&&... args) {
    return { new T{ std::forward<Args>(args)... } };
};

template<class T>
irf_ptr<T> irf_base<T>::shared_from_this() const {
    return irf_ptr<T>{ static_cast<T*>(this) };
}

template<class T, class Deleter>
irfd_ptr<T, Deleter> irfd_base<T, Deleter>::shared_from_this() const {
    return irfd_ptr<T, Deleter>{ static_cast<T*>(this) };
}

template<class U, class T>
irf_ptr<U> static_pointer_cast(const irf_ptr<T> &ptr) {
    return irf_ptr<U>{ static_cast<U*>(ptr.get()) };
};

template<class U, class T, class Deleter>
irfd_ptr<U, Deleter> static_pointer_cast(const irfd_ptr<T, Deleter> &ptr) {
    return irfd_ptr<U, Deleter>{ static_cast<U*>(ptr.get()) };
};

} /* namespace util */
} /* namespace borealis */

namespace std {

template<class T>
struct hash<borealis::util::irf_ptr<T>> {
    using element_t = borealis::util::irf_ptr<T>;

    size_t operator()(element_t e) {
        hash<T*> base;
        return base(e.get());
    }
};

template<class T, class Deleter>
struct hash<borealis::util::irfd_ptr<T, Deleter>> {
    using element_t = borealis::util::irfd_ptr<T, Deleter>;

    size_t operator()(element_t e) {
        hash<T*> base;
        return base(e.get());
    }
};


} /* namespace std */

namespace llvm {

template<class T> struct simplify_type< borealis::util::irf_ptr<T> > {
    typedef T* SimpleType;
    static SimpleType getSimplifiedValue(const borealis::util::irf_ptr<T>& Val) {
        return Val.get();
    }
};

template<class T> struct simplify_type< const borealis::util::irf_ptr<T> > {
    typedef T* SimpleType;
    static SimpleType getSimplifiedValue(const borealis::util::irf_ptr<T>& Val) {
        return Val.get();
    }
};

template<class T, class Deleter> struct simplify_type< borealis::util::irfd_ptr<T, Deleter> > {
    typedef T* SimpleType;
    static SimpleType getSimplifiedValue(const borealis::util::irfd_ptr<T, Deleter>& Val) {
        return Val.get();
    }
};

template<class T, class Deleter> struct simplify_type< const borealis::util::irfd_ptr<T, Deleter> > {
    typedef T* SimpleType;
    static SimpleType getSimplifiedValue(const borealis::util::irfd_ptr<T, Deleter>& Val) {
        return Val.get();
    }
};

} // namespace llvm

#include "Util/generate_unmacros.h"

#endif // IRF_PTR_HPP
