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
        for(const auto& m : val) {
            if(auto v = util::fromJson<T>(m)) {
                ret.push_back(*v);
            } else return nullptr;
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
        for(const auto& m : val) {
            if(auto v = util::fromJson<T>(m)) {
                ret.insert(*v);
            } else return nullptr;
        }
        return optional_ptr_t { new std::set<T>{std::move(ret)} };
    }
};

template<class V>
struct json_traits<std::map<std::string, V>> {
    typedef std::unique_ptr<std::map<std::string, V>> optional_ptr_t;

    static Json::Value toJson(const std::map<std::string, V>& val) {
        Json::Value ret;
        for(const auto& m : val) ret[m.first] = util::toJson(m.second);
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        std::map<std::string, V> ret;
        for(const auto& k : val.getMemberNames()) {
            if(auto v = util::fromJson<V>(val[k])) {
                ret[k] = *v;
            } else return nullptr;
        }
        return optional_ptr_t { new std::map<std::string, V>{std::move(ret)} };
    }
};

} // namespace util
} // namespace borealis

#endif /* JSON_TRAITS_HPP_ */
