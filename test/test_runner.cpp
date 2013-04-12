/*
 * test_runner.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include "Util/util.h"

#include "runner.h"
#include "wrapper.h"

namespace {

using namespace borealis;

TEST(Runner, noProgram) {
    {
        int res = borealis::Runner("foo")
        .withArg("bar")
        .run();

        ASSERT_EQ(E_EXEC_ERROR, res);
    }
}

TEST(Runner, noFile) {
    {
        int res = borealis::Runner("wrapper")
        .withArg("test/testcases/iniXX_XX.c")
        .run();

        ASSERT_EQ(E_GATHER_COMMENTS, res);
    }
}

TEST(Runner, basic) {
    {
        int res = borealis::Runner("wrapper")
        .withArg("test/testcases/ini01_01.c")
        .run();

        ASSERT_EQ(OK, res);
    }
}

} // namespace
