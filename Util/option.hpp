/*
 * option.hpp
 *
 *  Created on: Oct 25, 2012
 *      Author: belyaev
 */

#ifndef OPTION_HPP_
#define OPTION_HPP_

#include <functional>
#include <iostream>
#include <memory>
#include <utility>

#include "Util/copying_ptr.hpp"
#include "Util/type_traits.hpp"
#include "Util/json.hpp"

namespace borealis {
namespace util {

struct nothing_t {};

constexpr nothing_t nothing() {
    return nothing_t();
}

template< class OptionImpl >
struct option_iterator {
    OptionImpl* opt;
    option_iterator() : opt(nullptr) {};
    option_iterator(OptionImpl* opt) : opt(opt) {};
    option_iterator(const option_iterator&) = default;
    option_iterator& operator=(const option_iterator&) = default;

    bool operator==(const option_iterator& that) const {
        return opt == that.opt;
    }

    bool operator!=(const option_iterator& that) const {
        return opt != that.opt;
    }

    option_iterator operator++() {
        opt = nullptr;
        return *this;
    }

    option_iterator operator++(int) {
        auto* tmp = opt;
        opt = nullptr;
        return option_iterator(tmp);
    }

    typename OptionImpl::const_reference operator*() const {
        return opt->getUnsafe();
    }

    typename OptionImpl::reference operator*() {
        return opt->getUnsafe();
    }

    typename OptionImpl::const_pointer operator->() const {
        return opt->get();
    }

    typename OptionImpl::pointer operator->() {
        return opt->get();
    }
};

template< class OptionImpl >
struct const_option_iterator {
    const OptionImpl* opt;
    const_option_iterator() : opt(nullptr) {};
    const_option_iterator(const OptionImpl* opt) : opt(opt) {};
    const_option_iterator(const const_option_iterator&) = default;
    const_option_iterator& operator=(const const_option_iterator&) = default;

    bool operator==(const const_option_iterator& that) const {
        return opt == that.opt;
    }

    bool operator!=(const const_option_iterator& that) const {
        return opt != that.opt;
    }

    const_option_iterator operator++() {
        opt = nullptr;
        return *this;
    }

    const_option_iterator operator++(int) {
        auto* tmp = opt;
        opt = nullptr;
        return const_option_iterator(tmp);
    }

    typename OptionImpl::const_reference operator*() const {
        return opt->getUnsafe();
    }

    typename OptionImpl::const_pointer operator->() const {
        return opt->get();
    }
};

template<class T>
class option {
    typedef option self;

    copying_ptr<T> holder;

public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef option_iterator<self> iterator;
    typedef const_option_iterator<self> const_iterator;

    option() : holder{} {}
    option(nothing_t) : holder{} {}
    explicit option(const T& val) : holder{ new T(val) } {}
    explicit option(T&& val) : holder{ new T(val) } {}
    option(const self&) = default;
    option(self&&) = default;
    template<class It>
    option(It&& begin_, It&& end_) : holder{ (begin_ == end_) ? nullptr : new T(*begin_) } {}

    bool empty() const { return holder == nullptr; }

    void swap(self& that) noexcept {
        holder.swap(that.holder);
    }

    size_t hash() const noexcept {
        return std::hash<decltype(holder)>{}(holder);
    }

    self& operator=(nothing_t) {
        holder.reset();
        return *this;
    }
    self& operator=(const self& that) = default;
    self& operator=(self&& that) = default;

    void push_back(const T& val) {
        operator=(option(val));
    }

    bool operator==(const T& that) const {
        if( empty() ) return false;
        else return *holder == that;
    }

    friend bool operator==(const self& a, const self& b) {
        if( a.empty() ) return b.empty();
        else if( b.empty() ) return false;
        else return *a.holder == *b.holder;
    }

    bool operator!=(const T& that) const {
        return !operator==(that);
    }

    friend bool operator!=(const self& a, const self& b) {
        return !(a == b);
    }

    bool operator!() const {
        return empty();
    }

    explicit operator bool() const {
        return not empty();
    }

    friend std::ostream& operator<<(std::ostream& ost, const self& x) {
        for(auto&& el: x) return ost << "just(" << el << ")";
        return ost << "nothing";
    }

    const T* get() const {
        return holder.get();
    }

    T* get() {
        return holder.get();
    }

    template<class U>
    auto getOrElse(U&& def) -> common_type_t<U, T&> {
        auto* ptr = this->get();
        return ptr ? *ptr : std::forward<U>(def);
    }

    template<class U>
    auto getOrElse(U&& def) const -> common_type_t<U, const T&> {
        auto* ptr = this->get();
        return ptr ? *ptr : std::forward<U>(def);
    }

    const T& getUnsafe() const {
        return *holder;
    }

    T& getUnsafe() {
        return *holder;
    }

    T&& moveUnsafe() {
        return std::move(*holder);
    }

    const_iterator begin() const {
        if ( empty() ) return const_iterator();
        else return const_iterator(this);
    }

    const_iterator end() const {
        return const_iterator();
    }

    iterator begin() {
        if ( empty() ) return iterator();
        else return iterator(this);
    }

    iterator end() {
        return iterator();
    }
};

template<class T>
option<remove_const_reference_t<T>> just(T&& val) {
    return option<remove_const_reference_t<T>>{ std::forward<T>(val) };
}

template<class T>
inline option<T> nothing() {
    return nothing();
}

template<class T>
class option_ref {
    typedef option_ref self;

    T* holder;

public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef option_iterator<self> iterator;
    typedef const_option_iterator<self> const_iterator;

    option_ref() : holder(nullptr) {}
    option_ref(nothing_t) : holder(nullptr) {}
    explicit option_ref(T& val) : holder(&val) {}
    option_ref(const self& that): holder(that.holder) {}
    template<class It>
    option_ref(It&& begin_, It&& end_) : holder{ (begin_ == end_) ? nullptr : &*begin_ } {}


    bool empty() const { return holder == nullptr; }

    void swap(self& that) noexcept {
        std::swap(holder, that.holder);
    }

    size_t hash() const noexcept {
        return std::hash<decltype(holder)>{}(holder);
    }

    self& operator=(nothing_t) {
        self temp; // exception safety
        swap(temp);
        return *this;
    }
    self& operator=(const self& that) = default;
    self& operator=(self&& that) = default;

    void push_back(T& val) {
        operator=(option_ref(val));
    }

    bool operator==(const self& that) const {
        if( empty() ) return that.empty();
        else if( that.empty() ) return false;
        else return *holder == *(that.holder);
    }

    bool operator==(const T& that) const {
        if( empty() ) return false;
        else return *holder == that;
    }

    bool operator==(const nothing_t&) const {
        return empty();
    }

    bool operator!=(const self& that) const {
        return !operator==(that);
    }

    bool operator!=(const T& that) const {
        return !operator==(that);
    }

    bool operator!=(const nothing_t&) const {
        return !empty();
    }

    explicit operator bool() {
        return !empty();
    }

    bool operator!() const {
        return empty();
    }

    friend std::ostream& operator<<(std::ostream& ost, const self& x) {
        for(auto&& el: x) return ost << "just(" << el << ")";
        return ost << "nothing";
    }

    const T* get() const {
        return holder;
    }

    T* get() {
        return holder;
    }

    template<class U>
    auto getOrElse(U&& def) -> common_type_t<U, T&> {
        auto* ptr = this->get();
        return ptr ? *ptr : std::forward<U>(def);
    }

    template<class U>
    auto getOrElse(U&& def) const -> common_type_t<U, const T&> {
        auto* ptr = this->get();
        return ptr ? *ptr : std::forward<U>(def);
    }

    const T& getUnsafe() const {
        return *holder;
    }

    T& getUnsafe() {
        return *holder;
    }

    T&& moveUnsafe() {
        return std::move(*holder);
    }

    const_iterator begin() const {
        if ( empty() ) return const_iterator();
        else return const_iterator(this);
    }

    const_iterator end() const {
        return const_iterator();
    }

    iterator begin() {
        if ( empty() ) return iterator();
        else return iterator(this);
    }

    iterator end() {
        return iterator();
    }
};

template<class T>
option_ref<remove_reference_t<T>> justRef(T&& val) {
    return option_ref<remove_reference_t<T>>{ std::forward<T>(val) };
}

template<class T>
inline option_ref<T> nothingRef() {
    return nothing();
}

template<class T>
struct json_traits<option<T>> {
    typedef std::unique_ptr<option<T>> optional_ptr_t;
    using delegate = json_traits<T>;

    static Json::Value toJson(const option<T>& val) {
        for(auto&& el: val) return delegate::toJson(el);
        return Json::Value{};
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        if(json.isNull()) return optional_ptr_t{ new option<T>{nothing()} };
        auto del = delegate::fromJson(json);
        if(!del) return nullptr;
        else return optional_ptr_t{ new option<T>{just(*del)} };
    }
};

} // namespace util
} // namespace borealis

namespace std {

template<class T>
void swap(const borealis::util::option<T>& opt1,
          const borealis::util::option<T>& opt2) noexcept {
    opt1.swap(opt2);
    return;
}

template<class T>
void swap(const borealis::util::option_ref<T>& opt1,
          const borealis::util::option_ref<T>& opt2) noexcept {
    opt1.swap(opt2);
    return;
}

template<class T>
struct hash<borealis::util::option<T>> {
    size_t operator()(const borealis::util::option<T>& opt) const noexcept {
        return opt.hash();
    }
};

template<class T>
struct hash<borealis::util::option_ref<T>> {
    size_t operator()(const borealis::util::option_ref<T>& opt) const noexcept {
        return opt.hash();
    }
};

} // namespace std

#endif /* OPTION_HPP_ */
