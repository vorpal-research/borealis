/*
 * json_traits.hpp
 *
 *  Created on: Apr 12, 2013
 *      Author: belyaev
 */

#ifndef JSON_TRAITS_HPP_
#define JSON_TRAITS_HPP_

#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include "Util/json.hpp"

namespace borealis {
namespace util {

template<class T>
struct json_traits<std::vector<T>> {
    typedef std::unique_ptr<std::vector<T>> optional_ptr_t;

    static Json::Value toJson(const std::vector<T>& val) {
        Json::Value ret;
        for(const auto& m : val) ret.append(util::toJson(m));
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        std::vector<T> ret;
        if(!val.isArray()) return nullptr;
        for(const auto& m : val) {
            if(auto v = util::fromJson<T>(m)) {
                ret.push_back(*v);
            }
        }
        return optional_ptr_t { new std::vector<T>{std::move(ret)} };
    }
};

template<class T>
struct json_traits<std::set<T>> {
    typedef std::unique_ptr<std::set<T>> optional_ptr_t;

    static Json::Value toJson(const std::set<T>& val) {
        Json::Value ret;
        for(const auto& m : val) ret.append(util::toJson(m));
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        std::set<T> ret;
        if(!val.isArray()) return nullptr;
        for(const auto& m : val) {
            if(auto v = util::fromJson<T>(m)) {
                ret.insert(*v);
            }
        }
        return optional_ptr_t { new std::set<T>{std::move(ret)} };
    }
};

template<class T>
struct json_traits<std::unordered_set<T>> {
    typedef std::unique_ptr<std::unordered_set<T>> optional_ptr_t;

    static Json::Value toJson(const std::unordered_set<T>& val) {
        Json::Value ret;
        for(const auto& m : val) ret.append(util::toJson(m));
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        std::unordered_set<T> ret;
        if(!val.isArray()) return nullptr;
        for(const auto& m : val) {
            if(auto v = util::fromJson<T>(m)) {
                ret.insert(*v);
            }
        }
        return optional_ptr_t { new std::unordered_set<T>{std::move(ret)} };
    }
};

template<class K, class V, class Hash, class Equal, class Alloc>
struct json_traits<std::unordered_map<K, V, Hash, Equal, Alloc>> {
    using theMap_t = std::unordered_map<K, V, Hash, Equal, Alloc>;
    using optional_ptr_t = std::unique_ptr<theMap_t>;

    static Json::Value toJson(const theMap_t& val) {
        Json::Value ret = Json::arrayValue;
        for(const auto& kv : val) {
            Json::Value field = Json::objectValue;
            field["key"] = util::toJson(kv.first);
            field["value"] = util::toJson(kv.second);
            ret.append(field);
        }
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        theMap_t ret;
        if(!val.isArray()) return nullptr;
        for(const auto& kv : val) {
            if(!kv.isObject()) return nullptr;
            ret.insert(std::make_pair( util::fromJson<K>(kv["key"]), util::fromJson<V>(kv["value"]) ));
        }

        return optional_ptr_t { new theMap_t{ std::move(ret) } };
    }
};

} // namespace util
} // namespace borealis

#endif /* JSON_TRAITS_HPP_ */
