//
// Created by belyaev on 10/2/15.
//

#ifndef INDEXED_STRING_HPP
#define INDEXED_STRING_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "Util/string_ref.hpp"
#include "Util/json.hpp"
#include "Util/hash.hpp"

namespace borealis {
namespace util {

namespace impl_ {

struct string_cache {
    std::vector<std::string> fwd;
    std::unordered_map<string_ref, size_t> bwd;

    size_t operator[](string_ref key) {
        auto it = bwd.find(key);
        if(it != std::end(bwd)) {
            return it->second;
        } else {
            fwd.push_back(key.str());
            return bwd[fwd.back()] = fwd.size() - 1;
        }
    }

    const std::string& operator[](size_t key) {
        return fwd[key];
    }

};

} /* namespace impl_ */

class indexed_string {
    size_t id;

    static impl_::string_cache& cache_instance() {
        static impl_::string_cache cache;
        return cache;
    }

public:
    indexed_string(string_ref str): id(cache_instance()[str]) {}
    indexed_string(const std::string& str): id(cache_instance()[str]) {}
    indexed_string(const char* str): id(cache_instance()[str]) {}
    indexed_string(): indexed_string("") {}

    size_t hash() const {
        return hash::simple_hash_value(id);
    }

    string_ref view() const {
        return cache_instance()[id];
    }

    const std::string& str() const {
        return cache_instance()[id];
    }

    const char* c_str() const {
        return cache_instance()[id].c_str();
    }

    size_t size() const {
        return str().size();
    }

    bool empty() const {
        return str().empty();
    }

    friend bool operator==(indexed_string a, indexed_string b) {
        return a.id == b.id;
    }

    friend bool operator<(indexed_string a, indexed_string b) {
        return a.id < b.id;
    }

    friend std::ostream& operator<<(std::ostream& ost, indexed_string str) {
        return ost << str.str();
    }
};

template<>
struct json_traits<indexed_string> {
    using optional_ptr_t = std::unique_ptr<indexed_string>;

    static Json::Value toJson(const indexed_string& val) {
        Json::Value ret = Json::StaticString(val.c_str());
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& val) {
        if(!val.isString()) return nullptr;
        return std::make_unique<indexed_string>(val.asCString());
    }
};

} /* namespace util */
} /* namespace borealis */

namespace std {

template<>
struct hash<borealis::util::indexed_string> {
    size_t operator()(const borealis::util::indexed_string& v) const noexcept {
        return v.hash();
    }
};

} /* namespace std */

#endif //INDEXED_STRING_HPP
