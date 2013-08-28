/*
 * test_runner.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include "Util/util.h"

#include "Driver/runner.h"
#include "wrapper.h"

namespace {

using namespace borealis;
using namespace borealis::driver;

TEST(Runner, noProgram) {
    {
        int res = Runner("foo")
            .withArg("bar")
            .run();

        ASSERT_EQ(E_EXEC_ERROR, res);
    }
}

TEST(Runner, noFile) {
    {
        int res = Runner("wrapper")
            .withArg("test/testcases/XXX/iniXX_XX.c")
            .run();

        ASSERT_EQ(E_GATHER_COMMENTS, res);
    }
}

TEST(Runner, basic) {
    {
        int res = Runner("wrapper")
            .withArg("test/testcases/aegis/ini01_01.c")
            .run();

        ASSERT_EQ(OK, res);
    }
}

} // namespace
