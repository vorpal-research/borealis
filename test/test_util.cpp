/*
 * sample_test.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */

#include <vector>

#include "gtest/gtest.h"

#include "util.h"

namespace {

using namespace borealis::util;

using namespace borealis::util::streams;

TEST(Util, toString) {
	EXPECT_EQ(toString(23),"23");
	EXPECT_EQ(toString(true),"true");
	EXPECT_EQ(toString("foo"),"foo");

	void* ptr = reinterpret_cast<void*>(0xdeadbeef);
	EXPECT_EQ(toString(ptr),"0xdeadbeef");

	std::vector<int> vec{1,2,3,4};

	EXPECT_EQ(toString(vec),"[1, 2, 3, 4]");

}

TEST(Util, ltlt) {
	{
		std::string fill;
		llvm::raw_string_ostream ost(fill);


		ost << "Hello!" << endl;

		EXPECT_EQ(ost.str(),"Hello!\n");
	}

	{
		std::string fill;
		llvm::raw_string_ostream ost(fill);


		std::vector<int> vec{1,2,3,4};

		ost << vec;
		EXPECT_EQ(ost.str(),"[1, 2, 3, 4]");
	}

	{
		std::ostringstream ost;


		std::vector<int> vec{1,2,3,4};

		ost << vec;
		EXPECT_EQ(ost.str(),"[1, 2, 3, 4]");
	}


}

}

