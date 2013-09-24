/*
 * test_anno.cpp
 *
 *  Created on: Aug 16, 2013
 *      Author: belyaev
 */

#include <gtest/gtest.h>

#include "Util/cast.hpp"

#include "Anno/anno.hpp"
#include "Util/streams.hpp"

namespace {

using namespace borealis;
using namespace borealis::anno;
using namespace borealis::anno::calculator;

#define MK_ANNO_STR(name, ...) "@" #name " " #__VA_ARGS__ "\n"

TEST(Parse, annotation) {
    {
        using borealis::util::toString;

        ASSERT_EQ(toString(parse_command("/* @inline */").at(0)), "inline");

        auto cmd = parse_command(
            "/* "
            MK_ANNO_STR(ensures, (((x+1) == 42 && (x+2) == 67 && y)))
            MK_ANNO_STR(inline)
            MK_ANNO_STR(stack-depth, 0x00FF)
            "*/ "
        );

        ASSERT_EQ(cmd.size(), 3);

        ASSERT_EQ(toString(cmd[0]), "ensures(((((x + 1) == 42) && ((x + 2) == 67)) && y))");
        ASSERT_EQ(toString(cmd[1]), "inline");
        ASSERT_EQ(toString(cmd[2]), "stack-depth(255)");

        cmd = parse_command("// @ requires x | y");

        ASSERT_EQ(cmd.size(), 1);

        ASSERT_EQ(toString(cmd.at(0)), "requires((x | y))");

        ASSERT_EQ(toString(parse_command("/* @assert x */")               [0]), "assert(x)");
        ASSERT_EQ(toString(parse_command("/* @assert x+2 */")             [0]), "assert((x + 2))");
        ASSERT_EQ(toString(parse_command("/* @assert x(y) */")            [0]), "assert(x(y))");
        ASSERT_EQ(toString(parse_command("/* @assert x[y] */")            [0]), "assert(x[y])");
        ASSERT_EQ(toString(parse_command("/* @assert x[y+1] */")          [0]), "assert(x[(y + 1)])");
        ASSERT_EQ(toString(parse_command("/* @assert x[y(1, 2, 3)][2] */")[0]), "assert(x[y(1, 2, 3)][2])");
    }
}

} // namespace



