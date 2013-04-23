/*
 * test_wrapper.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include <fstream>
#include <set>
#include <string>
#include <vector>

#include "Passes/DefectManager/DefectInfo.h"
#include "Util/json.hpp"
#include "Util/json_traits.hpp"
#include "Util/util.h"

#include "runner.h"
#include "wrapper.h"

namespace {

using namespace borealis;
using namespace borealis::util;



static std::vector<std::string> getTestFiles(const std::string& dir, const std::string& file) {
    std::ifstream defs(dir + "/" + file);
    std::string filename;

    std::vector<std::string> tests;
    while(std::getline(defs, filename)) {
        if (filename[0] != '#') tests.push_back(dir + "/" + filename);
    }
    return tests;
}

static std::vector<std::string> ShortTestFiles(const std::string& dir) {
    return getTestFiles(dir, "tests.def");
}

static std::vector<std::string> LongTestFiles(const std::string& dir) {
    return getTestFiles(dir, "tests.long.def");
}



class WrapperTest : public ::testing::TestWithParam<std::string> {
public:
    virtual void SetUp() {
        inputF = GetParam();
        expectedF = inputF + ".expected";
        actualF = inputF + ".tmp";
    }
protected:
    std::string inputF;
    std::string expectedF;
    std::string actualF;
};



TEST_P(WrapperTest, basic) {
    int res = borealis::Runner("wrapper")
    .withArg("-cfgwrapper.tests.conf")
    .withArg("-opt-dump-output-file=" + actualF)
    .withArg(inputF)
    .run();

    ASSERT_EQ(OK, res);

    std::ifstream expectedS(expectedF);
    std::ifstream actualS(actualF);

    if (expectedS.fail()) {
        FAIL() << "Couldn't open file with expected results: " << expectedF;
    }

    if (actualS.fail()) {
        FAIL() << "Couldn't open file with actual results: " << actualF;
    }

    std::set<DefectInfo> expected;
    std::set<DefectInfo> actual;

    expectedS >> jsonify(expected);
    actualS >> jsonify(actual);

    EXPECT_EQ(expected, actual);
}



INSTANTIATE_TEST_CASE_P(Aegis, WrapperTest,    ::testing::ValuesIn(ShortTestFiles("test/testcases/aegis")));
INSTANTIATE_TEST_CASE_P(AegisLong, WrapperTest, ::testing::ValuesIn(LongTestFiles("test/testcases/aegis")));

INSTANTIATE_TEST_CASE_P(Contracts, WrapperTest,    ::testing::ValuesIn(ShortTestFiles("test/testcases/contracts")));
INSTANTIATE_TEST_CASE_P(ContractsLong, WrapperTest, ::testing::ValuesIn(LongTestFiles("test/testcases/contracts")));

INSTANTIATE_TEST_CASE_P(Misc, WrapperTest,    ::testing::ValuesIn(ShortTestFiles("test/testcases/misc")));
INSTANTIATE_TEST_CASE_P(MiscLong, WrapperTest, ::testing::ValuesIn(LongTestFiles("test/testcases/misc")));

INSTANTIATE_TEST_CASE_P(Necla, WrapperTest,    ::testing::ValuesIn(ShortTestFiles("test/testcases/necla")));
INSTANTIATE_TEST_CASE_P(NeclaLong, WrapperTest, ::testing::ValuesIn(LongTestFiles("test/testcases/necla")));



} // namespace
