/*
 * test_json.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>

#include "Util/json.hpp"
#include "Util/json_traits.hpp"
#include "Util/util.h"

#include "Logging/logger.hpp"

struct test_json_structure {
    int x;
    std::string str;
};

namespace borealis {
namespace util {

template<>
struct json_traits<test_json_structure> {
    typedef std::unique_ptr<test_json_structure> optional_ptr_t;

    static json::Value toJson(const test_json_structure& val) {
        json::Value dict;
        dict["x"] = val.x;
        dict["str"] = val.str;
        return dict;
    }

    static optional_ptr_t fromJson(const json::Value& json) {
        using borealis::util::json_object_builder;

        json_object_builder<test_json_structure, int, std::string> builder {
            "x", "str"
        };
        return optional_ptr_t {
            builder.build(json)
        };
    }
};

} /* namespace util */
} /* namespace borealis */

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

        EXPECT_EQ("foobar", v1);
        EXPECT_EQ(v0, v1);
    }

    {
        const long long v0 = 0xDEAD;
        int v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        EXPECT_EQ(0xDEAD, v1);
        EXPECT_EQ(v0, v1);
    }

    {
        long long v0 = 0xDEAD;
        int v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        EXPECT_EQ(0xDEAD, v1);
        EXPECT_EQ(v0, v1);
    }

    {
        const test_json_structure v0 = { 0xDEAD, "foobar" };
        test_json_structure v1;
        std::stringstream buf;

        buf << jsonify(v0);
        buf >> jsonify(v1);

        EXPECT_EQ(v0.x, v1.x);
        EXPECT_EQ(v0.str, v1.str);
    }

    {
        test_json_structure v0 { 20, "an object that is" };
        std::stringstream buf;

        buf << jsonify(std::string("totally not an object"));
        buf >> jsonify(v0);

        EXPECT_EQ(20, v0.x);
        EXPECT_EQ("an object that is", v0.str);
    }

    {
        test_json_structure v0 { 20, "an object that is" };
        json::Value v1;
        v1["str"] = "a different object";

        std::stringstream buf;

        buf << jsonify(v1);
        buf >> jsonify(v0);

        EXPECT_EQ(20, v0.x);
        EXPECT_EQ("an object that is", v0.str);
    }

    {
        test_json_structure v0 { 22, "stupid temporary value" };
        json::Value v1;
        v1["x"] = 20;
        v1["str"] = "an object that is";

        std::stringstream buf;

        buf << jsonify(v1);
        buf >> jsonify(v0);

        EXPECT_EQ(20, v0.x);
        EXPECT_EQ("an object that is", v0.str);
    }

}

} // namespace
