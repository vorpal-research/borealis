/*
 * sample_test.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */


#include "gtest/gtest.h"

#include "util.h"

namespace {

using namespace borealis::util;

using namespace borealis::util::streams;

TEST(UtilTest, Test_toString) {
	EXPECT_EQ(toString(23),"23");
	EXPECT_EQ(toString(true),"true");
	EXPECT_EQ(toString("foo"),"foo");

	int* ptr = (int*)(0xdeadbeef);
	EXPECT_EQ(toString(ptr),"0xdeadbeef");
}

TEST(UtilTest, Test_ltlt) {

	std::string fill;
	llvm::raw_string_ostream ost(fill);


	ost << "Hello!" << endl;

	EXPECT_EQ(ost.str(),"Hello!\n");
}

}

