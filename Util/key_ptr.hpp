/*
 * key_ptr.hpp
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef KEY_PTR_HPP_
#define KEY_PTR_HPP_

namespace borealis {
namespace util {

// smart ptr class similar to std::shared_ptr, but with value-based comparisons and hash
// intended to be used in maps and unordered maps
template<class T>
class key_holder_ptr {
    std::shared_ptr<T> inner;
public:
    key_holder_ptr(const std::shared_ptr<T>& cp) : inner(cp) {};
    key_holder_ptr(key_holder_ptr&&) = default;
    key_holder_ptr(const key_holder_ptr&) = default;

    T* get() { return inner.get(); }
    const T* get() const { return inner.get(); }

    bool operator==(const key_holder_ptr& that) const { return *inner == *that.inner; }
    bool operator<(const key_holder_ptr& that) const { return *inner < *that.inner; }

    T& operator*() { return *inner; }
    const T& operator*() const { return *inner; }

    std::shared_ptr<T>& operator->() { return inner; }
    const std::shared_ptr<T>& operator->() const { return inner; }
};

template<class T, class ...Args>
key_holder_ptr<T> make_key_holder(Args&&... args) {
    return key_holder_ptr<T>{ std::make_shared(std::forward(args)...) };
}

} // namespace util
} // namespace borealis

namespace std {

template<class T>
struct hash<borealis::util::key_holder_ptr<T>> {
    size_t operator()(const borealis::util::key_holder_ptr<T>& x) const {
        return std::hash<T>()(*x);
    }
};

}// namespace std

namespace borealis {
namespace util {

// ptr class adapter with value-based comparisons and hash
// intended to be used in maps and unordered maps
template<class T>
class key_ptr {
    const T* inner;
public:
    key_ptr(const T* cp) : inner(cp) {};
    key_ptr(const T& cp) : inner(&cp) {};
    key_ptr(T&&) = delete; // this is explicitly deleted
    // to drop common cases like key_ptr<string>("hello")
    // and key_ptr<string>(func_returning_string())
    // that are illegal but work with (const T&) case.
    // providing and deleting rvalue-ref constructor forbids these
    key_ptr(key_ptr&&) = default;
    key_ptr(const key_ptr&) = default;

    const T* get() const { return inner; }

    bool operator==(const key_ptr& that) const { return inner == that.inner || *inner == *that.inner; }
    bool operator<(const key_ptr& that) const { return *inner < *that.inner; }

    const T& operator*() const { return *inner; }
    const T* operator->() const { return inner; }
};

} // namespace util
} // namespace borealis

namespace std {

template<class T> struct hash<borealis::util::key_ptr<T>> {
    size_t operator()(const borealis::util::key_ptr<T>& x) const {
        return std::hash<T>()(*x);
    }
};

}// namespace std

#endif /* KEY_PTR_HPP_ */
