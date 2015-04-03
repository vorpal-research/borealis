//
// Created by belyaev on 4/3/15.
//

#ifndef CACHE_HPP
#define CACHE_HPP

#include <unordered_map>
#include <functional>

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

};

} /*namespace util*/
} /*namespace borealis*/


#endif // CACHE_HPP
