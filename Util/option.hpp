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

namespace borealis {
namespace util {

struct nothing_t {};

template<class T>
class option {
    typedef T* pointer;
    typedef option<T> self;

    std::shared_ptr<T> holder;

public:
    option() : holder(nullptr) {}
    option(nothing_t) : holder(nullptr) {}
    explicit option(const T& val) : holder(new T(val)) {}
    explicit option(const std::function<std::shared_ptr<T>()>& generator) : holder(generator()) {}
    option(const self& that) {
        if (that.empty()) {
            holder.reset();
        } else {
            const auto& val = *(that.holder);
            holder.reset(new T(val));
        }
    }
    option(self&& that) : holder(std::move(that.holder)) {}

    bool empty() const { return holder == nullptr; }

    void swap(self& that) {
        holder.swap(that.holder);
    }

    self& operator=(nothing_t) {
        self temp; // exception safety
        swap(temp);
        return *this;
    }
    self& operator=(const self& that) {
        self temp(that); // exception safety
        swap(temp);
        return *this;
    }
    self& operator=(self&& that) {
        holder = std::move(that.holder);
        return *this;
    }

    // this is provided only to support std::back_inserter iterator type
    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    void push_back(const T& val) {
        operator=(option(val));
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

    const T* get() const {
        return holder.get();
    }

    T* get() {
        return holder.get();
    }

    const T& getOrElse(const T& def) const {
        auto* ptr = holder.get();
        return ptr ? *ptr : def;
    }

    T getOrElse(T&& def) const {
        auto* ptr = holder.get();
        return ptr ? *ptr : def;
    }

    T& getOrElse(T& def) {
        auto* ptr = holder.get();
        return ptr ? *ptr : def;
    }

    const T& getUnsafe() const {
        return *holder.get();
    }

    T& getUnsafe() {
        return *holder.get();
    }

    struct option_iterator {
        self* opt;
        option_iterator() : opt(nullptr) {};
        option_iterator(self* opt) : opt(opt) {};
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

        const T& operator*() const {
            return opt->getUnsafe();
        }

        T& operator*() {
            return opt->getUnsafe();
        }

        const T* operator->() const {
            return opt->get();
        }

        T* operator->() {
            return opt->get();
        }
    };

    struct const_option_iterator {
        const self* opt;
        const_option_iterator() : opt(nullptr) {};
        const_option_iterator(const self* opt) : opt(opt) {};
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

        const T& operator*() const {
            return opt->getUnsafe();
        }

        const T* operator->() const {
            return opt->get();
        }
    };

    const_option_iterator begin() const {
        if (this->empty()) return const_option_iterator();
        else return const_option_iterator(this);
    }

    const_option_iterator end() const {
        return const_option_iterator();
    }

    option_iterator begin() {
        if (this->empty()) return option_iterator();
        else return option_iterator(this);
    }

    option_iterator end() {
        return option_iterator();
    }
};

template<class T>
option<T> just(const T& val) {
    return option<T>(val);
}

template<class T>
option<T> just(T&& val) {
    return option<T>(val);
}

inline nothing_t nothing() {
    return nothing_t();
}

template<class T>
inline option<T> nothing() {
    return nothing();
}

} // namespace util
} // namespace borealis

#endif /* OPTION_HPP_ */
