    /*
 * json.hpp
 *
 *  Created on: Apr 12, 2013
 *      Author: belyaev
 */

#ifndef JSON_HPP_
#define JSON_HPP_

#include <rapidjson/rapidjson.h>
#include <rapidjson/reader.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <json/json.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

#include "Util/meta.hpp"
#include "Util/sayonara.hpp"

#include "Util/macros.h"

namespace borealis {
namespace util {

namespace json {

enum class Type: uint8_t { null, boolean, signedInt, unsignedInt, floating, array, object, string, cstring };

class Value {
    Type type_ = Type::null;
    union {
        void* nullValue_ = nullptr;
        bool boolValue_;
        uint64_t unsignedValue_;
        int64_t signedValue_;
        double doubleValue_;
        std::string *stringValue_;
        std::pair<const char*, size_t>* cstringValue_;
        std::vector<Value> *arrayValue_;
        std::unordered_map<std::string, Value> *objectValue_;
    };

    template<class T, class ...Args>
    void init(T *&ptr, Args &&... args) { ptr = new T(std::forward<Args>(args)...); }

    void xvalue() {
        switch (type_) {
            case Type::null:
                nullValue_ = nullptr;
                break;
            case Type::boolean:
                boolValue_ = false;
                break;
            case Type::signedInt:
                signedValue_ = 0;
                break;
            case Type::unsignedInt:
                unsignedValue_ = 0;
                break;
            case Type::floating:
                doubleValue_ = 0.0;
                break;
            case Type::cstring:
                cstringValue_ = nullptr;
                break;
            case Type::string:
                stringValue_ = nullptr;
                break;
            case Type::array:
                arrayValue_ = nullptr;
                break;
            case Type::object:
                objectValue_ = nullptr;
                break;
            default:
                UNREACHABLE("Unknown object type detected")
                break;
        }
    }

    void init() {
        xvalue();
        switch (type_) {
            case Type::cstring:
                init(cstringValue_, "", 0);
                break;
            case Type::string:
                init(stringValue_, "");
                break;
            case Type::array:
                init(arrayValue_);
                break;
            case Type::object:
                init(objectValue_);
                break;
            default:
                break;
        }
    }

    template<class T>
    void destroy(T*& ptr) { delete ptr; }

    void destroy() {
        switch (type_) {
            case Type::cstring:
                destroy(cstringValue_);
                break;
            case Type::string:
                destroy(stringValue_);
                break;
            case Type::array:
                destroy(arrayValue_);
                break;
            case Type::object:
                destroy(objectValue_);
                break;
            default:
                break;
        }
        xvalue();
        type_ = Type::null;
        xvalue();
    }

    void swapSameType(Value& that) {
        using std::swap;
        ASSERTC(that.type_ == this->type_);
        switch (type_) {
            case Type::null:
                swap(nullValue_, that.nullValue_);
                break;
            case Type::boolean:
                swap(boolValue_, that.boolValue_);
                break;
            case Type::signedInt:
                swap(signedValue_, that.signedValue_);
                break;
            case Type::unsignedInt:
                swap(unsignedValue_, that.unsignedValue_);
                break;
            case Type::floating:
                swap(doubleValue_, that.doubleValue_);
                break;
            case Type::string:
                swap(stringValue_, that.stringValue_);
                break;
            case Type::cstring:
                swap(cstringValue_, that.cstringValue_);
                break;
            case Type::array:
                swap(arrayValue_, that.arrayValue_);
                break;
            case Type::object:
                swap(objectValue_, that.objectValue_);
                break;
            default:
                UNREACHABLE("unknown json::Value type detected")
                break;
        }
    }

    void moveFrom(Value& that) {
        destroy();
        type_ = that.type_;
        xvalue();
        swapSameType(that);
    }

    void copyFrom(const Value& that) {
        destroy();
        type_ = that.type_;
        switch (type_) {
            case Type::null:
                nullValue_ = that.nullValue_;
                break;
            case Type::boolean:
                boolValue_ = that.boolValue_;
                break;
            case Type::signedInt:
                signedValue_ = that.signedValue_;
                break;
            case Type::unsignedInt:
                unsignedValue_ = that.unsignedValue_;
                break;
            case Type::floating:
                doubleValue_ = that.doubleValue_;
                break;
            case Type::string:
                init(stringValue_, *that.stringValue_);
                break;
            case Type::cstring:
                init(cstringValue_, *that.cstringValue_);
                break;
            case Type::array:
                init(arrayValue_, *that.arrayValue_);
                break;
            case Type::object:
                init(objectValue_, *that.objectValue_);
                break;
            default:
                UNREACHABLE("Unknown type detected");
                break;
        }
    }

public:
    bool isNull() const { return type_ == Type::null; }
    bool isObject() const { return type_ == Type::object; }
    bool isString() const { return type_ == Type::string || type_ == Type::cstring; }
    bool isArray() const { return type_ == Type::array; }
    bool isFloating() const { return type_ == Type::floating; }
    bool isSigned() const { return type_ == Type::signedInt; }
    bool isUnsigned() const { return type_ == Type::unsignedInt; }
    bool isInteger() const { return isSigned() || isUnsigned(); }
    bool isNumber() const { return isInteger() || isFloating(); }
    bool isBool() const { return type_ == Type::boolean; }

    ~Value() { destroy(); }

    Value(): type_(Type::null) { init(); }
    Value(Type type) : type_(type) { init(); }
    Value(Value&& that): Value() {
        moveFrom(that);
    }
    Value(const Value& that): Value() {
        copyFrom(that);
    }
    Value& operator=(Value&& that) {
        moveFrom(that);
        return *this;
    }
    Value& operator=(const Value& that) {
        copyFrom(that);
        return *this;
    }
    /*
     * Booleans
     * */
    Value(bool b): type_(Type::boolean), boolValue_(b) {}
    Value& operator=(bool b){
        return *this = Value(b);
    }
    explicit operator bool() const { return boolValue_; }
    bool asBool() const { return isBool()? boolValue_ : false; }
    /*
     * Signed integers
     * */
    template<
        class Signed,
        std::enable_if_t<std::is_integral<Signed>::value && std::is_signed<Signed>::value, int> = 0
    >
    Value(Signed i): type_(Type::signedInt), signedValue_(i) {}
    template<
        class Signed,
        std::enable_if_t<std::is_integral<Signed>::value && std::is_signed<Signed>::value, int> = 0
    >
    Value& operator=(Signed i){
        return *this = Value(i);
    }
    template<
        class Signed,
        std::enable_if_t<std::is_integral<Signed>::value && std::is_signed<Signed>::value, int> = 0
    >
    explicit operator Signed() const {
        return isSigned() ? static_cast<Signed>(signedValue_) :
               isUnsigned() ? static_cast<Signed>(unsignedValue_) :
               isFloating() ? static_cast<Signed>(doubleValue_) : 0;
    }
    int asInt() const { return static_cast<int>(*this); }
    int64_t asInt64() const { return static_cast<int64_t>(*this); }
    /*
     * Unsigned integers
     * */
    template<
        class Unsigned,
        std::enable_if_t<std::is_integral<Unsigned>::value && std::is_unsigned<Unsigned>::value, int> = 0
    >
    Value(Unsigned i): type_(Type::unsignedInt), unsignedValue_(i) {}
    template<
        class Unsigned,
        std::enable_if_t<std::is_integral<Unsigned>::value && std::is_unsigned<Unsigned>::value, int> = 0
    >
    Value& operator=(Unsigned i){
        return *this = Value(i);
    }
    template<
        class Unsigned,
        std::enable_if_t<std::is_integral<Unsigned>::value && std::is_unsigned<Unsigned>::value, int> = 0
    >
    explicit operator Unsigned() const {
        return isSigned() ? static_cast<Unsigned>(signedValue_) :
               isUnsigned() ? static_cast<Unsigned>(unsignedValue_) :
               isFloating() ? static_cast<Unsigned>(doubleValue_) : 0;
    }
    unsigned asUnsignedInt() const { return static_cast<unsigned>(*this); }
    uint64_t asUnsignedInt64() const { return static_cast<uint64_t>(*this); }
    /*
     * FP
     * */
    template<
        class Floating,
        std::enable_if_t<std::is_floating_point<Floating>::value, int> = 0
    >
    Value(Floating f): type_(Type::floating), doubleValue_(f) {}
    template<
        class Floating,
        std::enable_if_t<std::is_floating_point<Floating>::value, int> = 0
    >
    Value& operator=(Floating f){
        return *this = Value(f);
    }
    template<
        class Floating,
        std::enable_if_t<std::is_floating_point<Floating>::value, int> = 0
    >
    explicit operator Floating() const {
        return isSigned() ? static_cast<Floating>(signedValue_) :
               isUnsigned() ? static_cast<Floating>(unsignedValue_) :
               isFloating() ? static_cast<Floating>(doubleValue_) : 0;
    }
    float asFloat() const { return static_cast<float>(*this); }
    double asDouble() const { return static_cast<double>(*this); }
    /*
     * Strings
     * */
    template<size_t N>
    Value(const char (&str)[N]): type_(Type::cstring), cstringValue_(new std::pair<const char*, size_t>{str, N - 1}) {}
    Value(const std::string& str): type_(Type::string), stringValue_(new std::string(str)) {}
    Value(std::string&& str): type_(Type::string), stringValue_(new std::string(std::move(str))) {}
    enum class StaticString{StaticString};
    static constexpr auto StaticString = StaticString::StaticString;
    Value(enum StaticString, const char* str): type_(Type::cstring), cstringValue_(new std::pair<const char*, size_t>{str, std::strlen(str)}) {}
    Value(enum StaticString, const char* str, size_t len): type_(Type::cstring), cstringValue_(new std::pair<const char*, size_t>{str, len}) {}
    template<size_t N>
    Value& operator=(const char (&str)[N]) {
        return *this = Value(str);
    }
    Value& operator=(const std::string& str) {
        return *this = Value(str);
    }
    Value& operator=(std::string&& str) {
        return *this = Value(std::move(str));
    }
    explicit operator const char*() const{
        switch(type_) {
            case Type::string: return stringValue_->c_str();
            case Type::cstring: return cstringValue_->first;
            default: return "";
        }
    }
    explicit operator std::string() const {
        switch(type_) {
            case Type::string: return *stringValue_;
            case Type::cstring: return std::string(cstringValue_->first, cstringValue_->second);
            default: return "";
        }
    }
    std::string asString() const { return static_cast<std::string>(*this); }
    const char* asCString() const { return static_cast<const char*>(*this); }
    size_t stringSize() const {
        switch(type_) {
            case Type::string: return stringValue_->size();
            case Type::cstring: return cstringValue_->second;
            default: return 0;
        }
    }
    /*
     * Arrays
     * */
    enum class Array{ Array };
    static constexpr auto Array = Array::Array;
    Value(enum Array): Value(Type::array) {}
    Value(std::vector<Value>&& v): type_(Type::array), arrayValue_(new std::vector<Value>(std::move(v))) {}
    template<
        class It,
        std::enable_if_t<std::is_constructible<Value, typename std::iterator_traits<It>::value_type>::value, int> = 0
    >
    Value(It begin, It end): type_(Type::array), arrayValue_(new std::vector<Value>(begin, end)) {}
    auto begin() {
        ASSERTC(isArray());
        return arrayValue_->begin();
    }
    auto end() {
        ASSERTC(isArray());
        return arrayValue_->end();
    }
    auto begin() const {
        ASSERTC(isArray());
        return arrayValue_->begin();
    }
    auto end() const {
        ASSERTC(isArray());
        return arrayValue_->end();
    }
    void ensureArray() {
        ASSERTC(isArray() || isNull())
        if(isNull() || arrayValue_ == nullptr) {
            type_ = Type::array;
            init();
        }
    }
    Value& operator[](size_t ix) {
        return arrayValue_->at(ix);
    }
    const Value& operator[](size_t ix) const {
        return arrayValue_->at(ix);
    }
    Value& operator[](int ix) {
        return arrayValue_->at(ix);
    }
    const Value& operator[](int ix) const {
        return arrayValue_->at(ix);
    }
    template<class Arg>
    void push_back(Arg&& v) {
        ensureArray();
        arrayValue_->emplace_back(std::forward<Arg>(v));
    }
    size_t arraySize() const{
        ASSERTC(isArray());
        return arrayValue_->size();
    }
    /*
     * Objects
     * */
    enum class Object{ Object };
    static constexpr auto Object = Object::Object;
    Value(enum Object): Value(Type::object) {}
    template<
        class It,
        std::enable_if_t<std::is_constructible<std::pair<std::string, Value>, typename std::iterator_traits<It>::value_type>::value, int> = 0
    >
    Value(It begin, It end): type_(Type::object), objectValue_(new std::unordered_map<std::string, Value>(begin, end)) {}
    void ensureObject() {
        ASSERTC(isObject() || isNull())
        if(isNull() || objectValue_ == nullptr) {
            type_ = Type::object;
            init();
        }
    }
    Value& operator[](const std::string& ix) {
        ensureObject();
        return (*objectValue_)[ix];
    }
    const Value& operator[](const std::string& ix) const {
        return (*objectValue_)[ix];
    }
    Value& operator[](const char* ix) {
        ensureObject();
        return (*objectValue_)[ix];
    }
    const Value& operator[](const char* ix) const {
        return (*objectValue_)[ix];
    }

    friend bool operator==(const Value& lhv, const Value& rhv) {
        if(lhv.isInteger() && rhv.isInteger()) {
            return lhv.asUnsignedInt64() == rhv.asUnsignedInt64();
        }
        if((lhv.isFloating() && rhv.isNumber())
            || (lhv.isNumber() && rhv.isFloating())) {
            return std::fabs(lhv.asDouble() - rhv.asDouble()) < std::numeric_limits<double>::epsilon() * 4;
        }

        if(lhv.isString() && rhv.isString()) {
            if(lhv.stringSize() != rhv.stringSize()) return false;
            return std::char_traits<char>::compare(lhv.asCString(), rhv.asCString(), lhv.stringSize()) == 0;
        }

        if(lhv.type_ != rhv.type_) return false;

        if(lhv.isArray()) return (*lhv.arrayValue_) == (*rhv.arrayValue_);
        if(lhv.isObject()) return (*lhv.objectValue_) == (*rhv.objectValue_);

        return true;
    }
    friend bool operator==(bool lhv, const Value& rhv) {
        return Value(lhv) == rhv;
    }
    friend bool operator==(const Value& lhv, bool rhv) {
        return lhv == Value(rhv);
    }
    friend bool operator==(int lhv, const Value& rhv) {
        return Value(lhv) == rhv;
    }
    friend bool operator==(const Value& lhv, int rhv) {
        return lhv == Value(rhv);
    }
    friend bool operator==(unsigned lhv, const Value& rhv) {
        return Value(lhv) == rhv;
    }
    friend bool operator==(const Value& lhv, unsigned rhv) {
        return lhv == Value(rhv);
    }
    friend bool operator==(const char* lhv, const Value& rhv) {
        return Value(StaticString, lhv) == rhv;
    }
    friend bool operator==(const Value& lhv, const char* rhv) {
        return lhv == Value(StaticString, rhv);
    }
    friend bool operator==(const std::string& lhv, const Value& rhv) {
        return Value(StaticString, lhv.c_str(), lhv.size()) == rhv;
    }
    friend bool operator==(const Value& lhv, const std::string& rhv) {
        return lhv == Value(StaticString, rhv.c_str(), rhv.size());
    }

    void swap(Value& that) {
        Value tmp = std::move(that);
        that = std::move(*this);
        *this = std::move(tmp);
    }

    template<class Handler>
    bool accept(Handler& h) const {
        switch (type_) {
            case Type::null:
                return h.Null();
            case Type::boolean:
                return h.Bool(boolValue_);
            case Type::signedInt:
                if(signedValue_ > std::numeric_limits<int>::max()
                    || signedValue_ < std::numeric_limits<int>::min()) {
                    return h.Int64(signedValue_);
                } else {
                    return h.Int(static_cast<int>(signedValue_));
                }
            case Type::unsignedInt:
                if(unsignedValue_ > std::numeric_limits<unsigned>::max()) {
                    return h.Uint64(unsignedValue_);
                } else {
                    return h.Uint(static_cast<unsigned>(unsignedValue_));
                }
            case Type::floating:
                return h.Double(doubleValue_);
            case Type::string:
            case Type::cstring:
                return h.String(asCString(), stringSize(), false);
            case Type::array: {
                bool res = true;
                res &= h.StartArray();
                if(!res) return false;
                for(auto&& e : *this) {
                    res &= e.accept(h);
                    if(!res) return false;
                }
                res &= h.EndArray(this->arraySize());
                if(!res) return false;
                return true;
            }
            case Type::object: {
                bool res = true;
                res &= h.StartObject();
                if(!res) return false;
                for(auto&& e : *this->objectValue_) {
                    res &= h.Key(e.first.c_str(), e.first.size(), false);
                    if(!res) return false;
                    res &= e.second.accept(h);
                    if(!res) return false;
                }
                res &= h.EndObject(this->objectValue_->size());
                if(!res) return false;
                return true;
            }
            default:
                UNREACHABLE("Unknown json::Value type")
        }
    }

};

struct ValueBuilder {
    std::stack<Value> stack;

    using Ch = char;

    template<class ...T>
    bool push(T&&... value) {
        stack.push(Value(std::forward<T>(value)...));
        return true;
    }

    Value pop() {
        auto v = std::move(stack.top());
        stack.pop();
        return std::move(v);
    }

    bool Null() {
        return push();
    }
    bool Bool(bool b){
        return push(b);
    }
    bool Int(int i) {
        return push(i);
    }
    bool Uint(unsigned i) {
        return push(i);
    }
    bool Int64(int64_t i) {
        return push(i);
    }
    bool Uint64(uint64_t i){
        return push(i);
    }
    bool Double(double d){
        return push(d);
    }
    /// enabled via kParseNumbersAsStringsFlag, string is not null-terminated (use length)
    bool RawNumber(const Ch* str, rapidjson::SizeType length, bool) {
        return push(std::stoll(std::string(str, length)));
    }
    bool String(const Ch* str, rapidjson::SizeType length, bool copy) {
        if(copy) {
            return push(std::string(str, length));
        } else {
            return push(Value(Value::StaticString, str, length));
        }
    }

    bool StartObject() {
        return push();
    }

    bool Key(const Ch* str, rapidjson::SizeType length, bool copy) {
        return String(str, length, copy);
    }
    bool EndObject(rapidjson::SizeType memberCount) {
        if(memberCount == 0) stack.top() = Value(Value::Object);
        Value ret{ Value::Object };
        while(memberCount != 0) {
            auto value = pop();
            auto key = pop();
            if(!key.isString()) return false;
            ret[key.asString()] = std::move(value);
            memberCount--;
        }
        stack.top() = std::move(ret);
        return true;
    }
    bool StartArray() {
        return push();
    }
    bool EndArray(rapidjson::SizeType elementCount) {
        if(elementCount == 0) stack.top() = Value(Value::Array);
        std::vector<Value> v;
        while(elementCount != 0) {
            v.push_back(pop());
            elementCount--;
        }
        std::reverse(std::begin(v), std::end(v));
        stack.top() = Value(std::move(v));
        return true;
    }
};

static std::istream& operator>>(std::istream& ist, Value& v) {
    ValueBuilder vb;
    rapidjson::IStreamWrapper istw(ist);
    rapidjson::Reader r;
    auto res = r.Parse(istw, vb);
    if(res) v = vb.pop();
    return ist;
}


static std::ostream& operator<<(std::ostream& ost, const Value& v) {
    rapidjson::OStreamWrapper ostw(ost);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> pw(ostw);
    v.accept(pw);
    return ost;
}

} /* namespace json */

////////////////////////////////////////////////////////////////////////////////

template<class T, typename SFINAE = void>
struct json_traits;

template<>
struct json_traits<json::Value> {
    typedef std::unique_ptr<json::Value> optional_ptr_t;

    static json::Value toJson(const json::Value& val) {
        return val;
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        return optional_ptr_t{ new json::Value{ json } };
    }
};

template<>
struct json_traits<bool> {
    typedef std::unique_ptr<bool> optional_ptr_t;

    static json::Value toJson(bool val) {
        return json::Value(val);
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        return json.isBool() ? optional_ptr_t{ new bool{json.asBool()} } :
                               nullptr;
    }
};

template< class T >
struct json_traits<T, GUARD(std::is_floating_point<T>::value)> {
    typedef std::unique_ptr<T> optional_ptr_t;

    static json::Value toJson(T val) {
        return json::Value(val);
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        return json.isNumber()
               ? optional_ptr_t{ new T{json.asDouble()} }
               : nullptr;
    }
};

template< class T >
struct json_traits<T, GUARD(std::is_integral<T>::value && std::is_signed<T>::value)> {
    typedef std::unique_ptr<T> optional_ptr_t;

    static json::Value toJson(T val) {
        return json::Value(val);
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        return json.isInteger() ? optional_ptr_t{ new T{ json.asInt() } } :
                                   nullptr;
    }
};

template< class T >
struct json_traits<T, GUARD(std::is_integral<T>::value && std::is_unsigned<T>::value)> {
    typedef std::unique_ptr<T> optional_ptr_t;

    static json::Value toJson(T val) {
        return json::Value(val);
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        return json.isInteger() ? optional_ptr_t{ new T{ json.asUnsignedInt() } } :
                                   nullptr;
    }
};

template<>
struct json_traits<std::string> {
    typedef std::unique_ptr<std::string> optional_ptr_t;

    static json::Value toJson(const std::string& val) {
        return json::Value(val);
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        return json.isString()
               ? optional_ptr_t{ new std::string{json.asString()} }
               : nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////

template<class T>
json::Value toJson(const T& val) {
    return json_traits<T>::toJson(val);
}

template<class T>
typename json_traits<T>::optional_ptr_t fromJson(const json::Value& json) {
    return json_traits<T>::fromJson(json);
}

template<class K>
std::decay_t<K> fromJsonWithDefault(const json::Value& json, K&& orDefault) {
    if(auto ptr = util::fromJson<std::decay_t<K>>(json)) {
        return std::move(*ptr);
    } else return std::forward<K>(orDefault);
}

template<class K>
bool assignJson(K& where, const json::Value& json) {
    if(auto ptr = util::fromJson<std::decay_t<K>>(json)) {
        where = std::move(*ptr);
        return true;
    } else return false;
}

template<class T>
typename json_traits<T>::optional_ptr_t read_as_json(std::istream& ist) {
    json::Value use;
    ist >> use;
    return util::fromJson<T>(use);
}

template<class T>
void write_as_json(std::ostream& ost, const T& val) {
    ost << util::toJson(val);
}

////////////////////////////////////////////////////////////////////////////////

template<class T>
struct Jsoner {
    T* value;

    friend std::ostream& operator<< (std::ostream& ost, Jsoner<T> json) {
        write_as_json(ost, *json.value);
        return ost;
    }

    friend std::istream& operator>> (std::istream& ist, Jsoner<T> json) {
        auto read = read_as_json<T>(ist);
        if(read) *json.value = *read;
        else ist.clear(std::ios::failbit);
        return ist;
    }

};

template<class T>
struct const_Jsoner {
    const T* value;

    friend std::ostream& operator<< (std::ostream& ost, const_Jsoner<T> json) {
        write_as_json(ost, *json.value);
        return ost;
    }
};

template<class T>
Jsoner<T> jsonify(T& value) { return Jsoner<T>{ &value }; }
template<class T>
const_Jsoner<T> jsonify(const T& value) { return const_Jsoner<T>{ &value }; }

////////////////////////////////////////////////////////////////////////////////

namespace impl {
static bool allptrs() {
    return true;
}
template<class H, class ...T>
static bool allptrs(const H& h, const T&... t) {
    if(!h) return false;
    else return allptrs(t...);
}
} //namespace impl

template<class Obj, class ...Args>
struct json_object_builder {
    std::vector<std::string> keys;

    // this is a constructor that takes a number of std::strings
    // exactly the same as the number of types in Args
    json_object_builder(const type_K_comb_q<std::string, Args>&... keys) : keys { keys... } {}

    template<size_t Ix>
    using type_arg_at = util::index_in_row_q<Ix, Args...>;

    template <size_t Ix>
    std::unique_ptr<type_arg_at<Ix>> buildSingle(const json::Value& val) const {
        return util::fromJson<type_arg_at<Ix>>(val[keys[Ix]]);
    };

    template<size_t ...Ixs>
    Obj* build_(const json::Value& val, util::indexer<Ixs...>) const {
        if(!val.isObject()) return nullptr;

        auto ptrs = std::make_tuple(
            buildSingle<Ixs>(val)...
        );

        if(!impl::allptrs(std::get<Ixs>(ptrs)...)) return nullptr;

        return new Obj {
            std::move(*std::get<Ixs>(ptrs))...
        };
    }

    Obj* build(const json::Value& val) const {
        return build_(val, typename util::make_indexer<Args...>::type());
    }
};

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"

#endif /* JSON_HPP_ */
