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

template<class T>
class option {
    typedef T* pointer;
    typedef option<T> self;

    std::shared_ptr<T> holder;

public:
    option() : holder(nullptr) {}
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

    void swap(const self& that ) {
        holder.swap(that.holder);
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

    bool operator==(const self& that) const {
        if( empty() ) {
            return that.empty() ? true : false;
        }
        else if( that.empty() ) return false;
        else return *holder == *(that.holder);
    }

    bool operator==(const T& that) const {
        if( empty() ) return false;
        else return *holder == that;
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

    T& getOrElse(T& def) {
        auto* ptr = holder.get();
        return ptr ? *ptr : def;
    }

    struct option_iterator {
        const self* opt;
        option_iterator(const self* opt) : opt(opt){};
        option_iterator(const option_iterator&) = default;
        option_iterator& operator=(const option_iterator&) = default;

        option_iterator& operator==(const option_iterator& that) {
            return opt == that.opt;
        }

        option_iterator& operator!=(const option_iterator& that) {
            return opt != that.opt;
        }

        option_iterator operator++() {
            opt = nullptr;
            return option_iterator(nullptr);
        }

        option_iterator operator++(int) {
            auto* tmp = opt;
            opt = nullptr;
            return option_iterator(tmp);
        }
    };

    option_iterator begin() {
        return option_iterator(this);
    }

    option_iterator end() {
        return option_iterator(nullptr);
    }
};

template<class T>
option<T> just(const T& val) {
    return option<T>(val);
}

template<class T>
option<T> just(T && val) {
    return option<T>(val);
}

template<class T>
option<T> nothing() {
    return option<T>();
}

} // namespace util
} // namespace borealis

#endif /* OPTION_HPP_ */
