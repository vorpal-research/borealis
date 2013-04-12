/*
 * json_traits.hpp
 *
 *  Created on: Apr 12, 2013
 *      Author: belyaev
 */

#ifndef JSON_TRAITS_HPP_
#define JSON_TRAITS_HPP_

#include <vector>
#include <map>

#include "Util/json.hpp"

namespace borealis {
namespace util {

template<class T>
struct json_traits<std::vector<T>> {
    typedef std::unique_ptr<std::vector<T>> optional_ptr_t;

    static Json::Value toJson(const std::vector<T>& val) {
        Json::Value ret;
        for(const auto& v : val) ret.append(v);
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        std::vector<T> ret;
        for(const auto& v : val) ret.push_back(v);
        return ret;
    }
};

template<class V>
struct json_traits<std::map<std::string, V>> {
    typedef std::unique_ptr<std::map<std::string, V>> optional_ptr_t;

    static Json::Value toJson(const std::map<std::string, V>& val) {
        Json::Value ret;
        for(const auto& v : val) {
            ret[v.first] = toJson(v.second);
        }
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        std::map<std::string, V> ret;

        for(const auto& k : val.getMemberNames()) {
            if(auto v = util::fromJson<V>(val[k])) {
                ret[k] = v;
            } else return nullptr;
        }

        return optional_ptr_t{ new std::map<std::string, V>(std::move(ret)) };
    }
};

} // namespace util
} // namespace borealis



#endif /* JSON_TRAITS_HPP_ */
