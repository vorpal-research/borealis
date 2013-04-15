/*
 * test_wrapper.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include <fstream>
#include <string>
#include <vector>

#include "Util/util.h"

#include "runner.h"
#include "wrapper.h"

namespace {

using namespace borealis;

static std::vector<std::string> getTestFiles(const std::string& dir) {
    std::ifstream defs(dir + "/tests.def");
    std::string filename;

    std::vector<std::string> tests;
    while(std::getline(defs, filename)) {
        if (filename[0] != '#') tests.push_back(dir + "/" + filename);
    }
    return tests;
}

class WrapperTest : public ::testing::TestWithParam<std::string> {
public:
    virtual void SetUp() {
        filename = GetParam();
    }
protected:
    std::string filename;
};

TEST_P(WrapperTest, basic) {
    int res = borealis::Runner("wrapper")
    .withArg(filename)
    .run();

    ASSERT_EQ(OK, res);
}

INSTANTIATE_TEST_CASE_P(Aegis, WrapperTest, ::testing::ValuesIn(getTestFiles("test/testcases/aegis")));

} // namespace
