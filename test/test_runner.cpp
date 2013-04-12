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

TEST(Runner, invalidProgram) {
    {
        int res = borealis::Runner("foo")
        .withArg("bar")
        .run();

        ASSERT_EQ(E_PROGRAM_NOT_FOUND, res);
    }
}

} // namespace
