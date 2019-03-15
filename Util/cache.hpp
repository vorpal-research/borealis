//
// Created by belyaev on 4/3/15.
//

#ifndef CACHE_HPP
#define CACHE_HPP

#include <unordered_map>
#include <functional>
#include <memory>

namespace borealis{
namespace util{

template<class Key, class Value>
using DefaultHashCacheImpl = std::unordered_map<Key, Value>;

template<class Key, class Value, template <class,class> class Impl = DefaultHashCacheImpl>
class cache {
    std::function<Value(Key)> generator;
    mutable Impl<Key, Value> container;
public:
    cache() = default;
    cache(const cache&) = default;
    cache(cache&&) = default;

    cache& operator=(cache&&) = default;
    cache& operator=(const cache&) = default;

    template<class Generator>
    cache(Generator generator): generator(generator), container{} {};

    Value& at(const Key& key) {
        auto it = container.find(key);
        if(it == std::end(container)) it = container.insert({key, generator(key)}).first;
        return it->second;
    }

    const Value& at(const Key& key) const {
        auto it = container.find(key);
        if(it == std::end(container)) it = container.insert({key, generator(key)}).first;
        return it->second;
    }

    Value& operator[](const Key& key) { return at(key); }
    const Value& operator[](const Key& key) const { return at(key); }
    size_t size() const { return container.size(); }

};


template <class Key, class Value, class Impl>
class cacheWImpl {
    std::function<Value(Key)> generator;
    mutable Impl container;
public:
    cacheWImpl() = default;

    cacheWImpl(const cacheWImpl&) = default;

    cacheWImpl(cacheWImpl&&) = default;

    cacheWImpl& operator=(cacheWImpl&&) = default;

    cacheWImpl& operator=(const cacheWImpl&) = default;

    template <class Generator>
    cacheWImpl(Generator generator): generator(generator), container{} {};

    Value& at(const Key& key) {
        auto it = container.find(key);
        if (it == std::end(container)) it = container.insert({key, generator(key)}).first;
        return it->second;
    }

    const Value& at(const Key& key) const {
        auto it = container.find(key);
        if (it == std::end(container)) it = container.insert({key, generator(key)}).first;
        return it->second;
    }

    Value& operator[](const Key& key) { return at(key); }

    const Value& operator[](const Key& key) const { return at(key); }

    size_t size() const { return container.size(); }

};

template<class Key, class Generator, class Value = decltype(std::declval<Generator>(std::declval<Key>()))>
cache<Key, Value, DefaultHashCacheImpl> make_default_cache(Generator gen) {
    return { gen };
}

namespace impl_ {
    template<class T>
    struct sequence_generator {
        T value;

        sequence_generator(): value{} {};

        template<class Arg>
        T operator()(Arg&&) {
            return value++;
        }
    };
}

template<class Key, class Value = size_t>
cache<Key, Value, DefaultHashCacheImpl> make_size_t_cache() {
    return { impl_::sequence_generator<Value>{} };
}

template<class Value, class ...Args>
cache<std::tuple<Args...>, Value, DefaultHashCacheImpl> cached_make_shared() {
    return { as_packed([](Args&&... args){ return std::make_shared<Value>(std::forward<Args>(args)...); }) };
};

} /*namespace util*/
} /*namespace borealis*/


#endif // CACHE_HPP
