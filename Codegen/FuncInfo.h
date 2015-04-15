/*
 * FuncInfo.h
 *
 *  Created on: Jan 14, 2013
 *      Author: belyaev
 */

#ifndef FUNCINFO_H_
#define FUNCINFO_H_

#include "Util/json.hpp"
#include "Util/option.hpp"
#include "Util/util.h"

#include "Codegen/libFunc.h"

#include "Util/macros.h"
#include "Util/generate_macros.h"

namespace borealis {

namespace func_info {

GENERATE_FANCY_ENUM(SpecialTag, Malloc, None);
GENERATE_FANCY_ENUM(AccessPatternTag, Read, Write, ReadWrite, Delete, None);
GENERATE_FANCY_ENUM(ArrayTag, IsArray, IsNotArray);

struct ResultInfo {
    SpecialTag special = SpecialTag::None;
    util::option<size_t> sizeArgument = util::nothing();
    ArrayTag isArray = ArrayTag::IsNotArray;
};

struct ArgInfo {
    AccessPatternTag access = AccessPatternTag::None;
    util::option<size_t> sizeArgument = util::nothing();
    ArrayTag isArray = ArrayTag::IsNotArray;
};

struct FuncInfo {
    std::string id;
    std::string signature;
    ResultInfo resultInfo;
    std::vector<ArgInfo> argInfo;
};

} /* namespace func_info */
} // namespace borealis

namespace borealis {
namespace util {

template<>
struct json_traits<borealis::func_info::ResultInfo> {
    using value_type = borealis::func_info::ResultInfo;
    using optional_ptr_t = std::unique_ptr<value_type>;

    static Json::Value toJson(const value_type& val) {
        using namespace borealis::func_info;

        Json::Value ret = Json::objectValue;
        ret["special"] = util::toJson(val.special);
        ret["size"] = util::toJson(val.sizeArgument);
        ret["array"] = util::toJson(val.isArray == ArrayTag::IsArray);
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using namespace borealis::func_info;
        if(!json.isObject()) return nullptr;

        auto retVal = util::make_unique<value_type>();
        util::assignJson(retVal->special, json["special"]);
        util::assignJson(retVal->sizeArgument, json["size"]);
        retVal->isArray = util::fromJsonWithDefault(json["array"], false)? ArrayTag::IsArray : ArrayTag::IsNotArray;
        return std::move(retVal);
    }
};

template<>
struct json_traits<borealis::func_info::ArgInfo> {
    using value_type = borealis::func_info::ArgInfo;
    using optional_ptr_t = std::unique_ptr<value_type>;

    static Json::Value toJson(const value_type& val) {
        using namespace borealis::func_info;

        Json::Value ret = Json::objectValue;
        ret["access"] = util::toJson(val.access);
        ret["size"] = util::toJson(val.sizeArgument);
        ret["array"] = util::toJson(val.isArray == ArrayTag::IsArray);
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using namespace borealis::func_info;
        if(!json.isObject()) return nullptr;

        auto retVal = util::make_unique<value_type>();
        assignJson(retVal->access, json["access"]);
        assignJson(retVal->sizeArgument, json["size"]);
        retVal->isArray = util::fromJsonWithDefault(json["array"], false)? ArrayTag::IsArray : ArrayTag::IsNotArray;
        return std::move(retVal);
    }
};

template<>
struct json_traits<borealis::func_info::FuncInfo> {
    using value_type = borealis::func_info::FuncInfo;
    using optional_ptr_t = std::unique_ptr<value_type>;

    static Json::Value toJson(const value_type& val) {
        using namespace borealis::func_info;

        Json::Value ret = Json::objectValue;
        ret["name"] = util::toJson(val.id);
        ret["signature"] = util::toJson(val.signature);
        ret["result"] = util::toJson(val.resultInfo);
        ret["args"] = util::toJson(val.argInfo);
        return ret;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using namespace borealis::func_info;
        if(!json.isObject()) return nullptr;
        if(!json["name"].isString()) return nullptr;
        if(!json["signature"].isString()) return nullptr;

        auto retVal = util::make_unique<value_type>();

        util::assignJson(retVal->id, json["name"]);
        util::assignJson(retVal->signature, json["signature"]);
        util::assignJson(retVal->resultInfo, json["result"]);
        util::assignJson(retVal->argInfo, json["args"]);

        return std::move(retVal);
    }
};

} /* namespace util */
} /* namespace borealis */

GENERATE_OUTLINE_ENUM_JSON_TRAITS(borealis::func_info::SpecialTag, Malloc, None);
GENERATE_OUTLINE_ENUM_JSON_TRAITS(borealis::func_info::AccessPatternTag, Read, Write, ReadWrite, Delete, None);
GENERATE_OUTLINE_ENUM_JSON_TRAITS(borealis::func_info::ArrayTag, IsArray, IsNotArray);

#include "Util/generate_unmacros.h"
#include "Util/unmacros.h"

#endif /* FUNCINFO_H_ */
