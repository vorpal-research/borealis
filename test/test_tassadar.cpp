/*
 * test_wrapper.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include <fstream>
#include <unordered_set>
#include <string>
#include <vector>


#include "Driver/gestalt.h"
#include "Driver/runner.h"
#include "Passes/Defect/DefectManager/DefectInfo.h"
#include "Util/json.hpp"
#include "Util/json_traits.hpp"
#include "Util/util.h"


namespace {

using namespace borealis;
using namespace borealis::util;
using namespace borealis::driver;



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
    return getTestFiles(dir, "tests.def.long");
}



class TassadarTest : public ::testing::TestWithParam<std::string> {
public:
    virtual void SetUp() {
        inputF = GetParam();
        expectedF = inputF + ".expected";
        actualF = inputF + ".tmp";
        paramF = inputF + ".params";
    }
protected:
    std::string inputF;
    std::string expectedF;
    std::string actualF;
    std::string paramF;
};



TEST_P(TassadarTest, basic) {

    std::vector<std::string> additionalArgs;

    std::ifstream paramS(paramF);
    while (paramS.good()) {
        std::string arg;
        std::getline(paramS, arg);
        additionalArgs.push_back(arg);
    }

    int res = Runner("wrapper")
        .withArg("---config:wrapper.tassadar.conf")
        .withArg("---output:tassadar-output-file:" + actualF)
        .withArgs(additionalArgs)
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

    std::unordered_set<util::option<std::string>> expected;
    std::unordered_set<util::option<std::string>> actual;

    expectedS >> jsonify(expected);
    actualS >> jsonify(actual);

    EXPECT_EQ(expected, actual);

}

std::string GetTestName(const std::string& param) {
    auto slash = param.find_last_of('/');
    return slash == std::string::npos ? param : param.substr(slash + 1);
}


INSTANTIATE_NAMED_TEST_CASE_P(Tassadar, TassadarTest,
    ::testing::ValuesIn(ShortTestFiles("test/testcases/tassadar")), GetTestName);
INSTANTIATE_NAMED_TEST_CASE_P(TassadarLong, TassadarTest,
    ::testing::ValuesIn(LongTestFiles("test/testcases/tassadar")), GetTestName);




//INSTANTIATE_TEST_CASE_P(SvComp, WrapperTest,    ::testing::ValuesIn(ShortTestFiles("test/testcases/svcomp")));
//INSTANTIATE_TEST_CASE_P(SvCompLong, WrapperTest, ::testing::ValuesIn(LongTestFiles("test/testcases/svcomp")));


} // namespace
