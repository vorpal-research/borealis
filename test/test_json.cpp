/*
 * test_cast.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>

#include "Util/util.h"
#include "Util/json.hpp"
#include "Util/json_traits.hpp"

#include "Logging/logger.hpp"

struct test_json_structure {
    int x;
    std::string str;
};

namespace borealis{ namespace util {

template<>
struct json_traits<test_json_structure> {
    typedef std::unique_ptr<test_json_structure> optional_ptr_t;

    static Json::Value toJson(const test_json_structure& val) {
        Json::Value dict;
        dict["x"] = val.x;
        dict["str"] = val.str;
        return dict;
    }

    static optional_ptr_t fromJson(const Json::Value& json) {
        using borealis::util::json_object_builder;

        json_object_builder<test_json_structure, int, std::string> builder {
            "x", "str"
        };
        return optional_ptr_t{
            builder.build(json)
        };
    }

};

} /* namespace util */ } /* namespace borealis */

namespace {

using namespace borealis;
using namespace borealis::logging;
using namespace borealis::util;

TEST(Json, readwrite) {
    using borealis::util::jsonify;

    {
        std::string v0 = "foobar";
        std::string v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        ASSERT_EQ(v0, v1);
        ASSERT_EQ(v1, "foobar");

    }

    {
        const long long v0 = 0xDEAD;
        int v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        ASSERT_EQ(v0, v1);
        ASSERT_EQ(v1, 0xDEAD);
    }

    {
        long long v0 = 0xDEAD;
        int v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        ASSERT_EQ(v0, v1);
        ASSERT_EQ(v1, 0xDEAD);
    }

    {
        const test_json_structure v0 = { 0xDEAD, "foobar" };
        test_json_structure v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        ASSERT_EQ(v0.x, v1.x);
        ASSERT_EQ(v0.str, v1.str);
    }

    {
        test_json_structure v0 { 20, "an object that is" };
        std::stringstream buf;

        buf << jsonify(std::string("totally not an object"));
        buf >> jsonify(v0);

        ASSERT_EQ(v0.x, 20);
        ASSERT_EQ(v0.str, "an object that is");
    }

    {
        test_json_structure v0 { 20, "an object that is" };
        Json::Value v1;
        v1["str"] = "a different object";

        std::stringstream buf;

        buf << jsonify(v1);
        buf >> jsonify(v0);

        ASSERT_EQ(v0.x, 20);
        ASSERT_EQ(v0.str, "an object that is");
    }

    {
        test_json_structure v0 { 22, "stupid temporary value" };
        Json::Value v1;
        v1["x"] = 20;
        v1["str"] = "an object that is";

        std::stringstream buf;

        buf << jsonify(v1);
        buf >> jsonify(v0);

        ASSERT_EQ(v0.x, 20);
        ASSERT_EQ(v0.str, "an object that is");
    }

    std::vector<std::string> tmp{"A", "B", "C", "D"};
    infos () << jsonify(tmp);
    infos () << tmp;
}

} // namespace
