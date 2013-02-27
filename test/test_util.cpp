/*
 * sample_test.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */

#include <vector>

#include <gtest/gtest.h>

#include "Util/util.h"
#include "Util/iterators.hpp"

namespace {

using namespace borealis;
using namespace borealis::util;
using namespace borealis::util::streams;

TEST(Util, views) {

	{
		std::vector<int> f{1,2,3,4};
		EXPECT_EQ(head(f), 1);

		std::ostringstream ost;
		for_each(tail(f), [&](int v){ ost << v; });
		EXPECT_EQ(ost.str(), "234");
	}

	{
		int arr[] = {22,23,24,25};
		std::ostringstream ost;
		for_each(view(arr,arr+4), [&](int v){ ost << v; });
		EXPECT_EQ(ost.str(), "22232425");
	}

	{
		struct fail_on_copy {
			int numcopies;
			fail_on_copy(): numcopies(0){};
			fail_on_copy(const fail_on_copy& that): numcopies(that.numcopies) {
				numcopies++;
				if(numcopies > 2) ADD_FAILURE() << "The copy func copies more than once!";
			}
		};
		std::vector<fail_on_copy> checker { fail_on_copy(), fail_on_copy(), fail_on_copy() };
		std::vector<fail_on_copy> cp = copy(checker);
	}

	{
		std::vector<int> v1 { 1,2,3,4 };
		std::vector<int> v2 = copy(v1);
		EXPECT_EQ(v1, v2);

		v1[0] = 42;

		EXPECT_EQ(v1[0], 42);
		EXPECT_NE(v1[0], v2[0]);
		EXPECT_NE(v1, v2);
	}

}

TEST(Util, toString) {
	EXPECT_EQ(toString(23),"23");
	EXPECT_EQ(toString(true),"true");
	EXPECT_EQ(toString("foo"),"foo");

	void* ptr = reinterpret_cast<void*>(0xdeadbeef);
	EXPECT_EQ(toString(ptr),"0xdeadbeef");

	std::vector<int> vec{1,2,3,4};
	EXPECT_EQ(toString(vec),"[1, 2, 3, 4]");

	std::set<int> st{1,2,3,4};
	EXPECT_EQ(toString(st),"(1, 2, 3, 4)");
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

TEST(Util, option) {
    {

        option<int> op = just(2);

        EXPECT_EQ(op.getUnsafe(), 2);
        EXPECT_FALSE(op.empty());

        int count = 0;
        for(auto& i: op) {
            count++;
            EXPECT_EQ(i, 2);
        }

        EXPECT_EQ(count, 1);

        option<int> nop = nothing();
        EXPECT_TRUE(nop.empty());

        count = 0;
        for(auto& i: nop) {
            count++;
            EXPECT_EQ(i, 2);
        }

        EXPECT_EQ(count, 0);

        {
            auto arg0 = just(1);
            auto arg1 = just(2);
            // this will fail if res is empty
            auto res = just(0);
            std::transform(
                    arg0.begin(),
                    arg0.end(),
                    arg1.begin(),
                    res.begin(),
                    [](int a, int b){ return a+b; }
            );

            EXPECT_NE(res, just(0));
            EXPECT_EQ(res, just(3));
        }

        {
            auto arg0 = just(1);
            auto arg1 = just(2);
            // this will not fail if res is empty
            // cos we use back_inserter
            auto res = nothing<int>();
            std::transform(
                arg0.begin(),
                arg0.end(),
                arg1.begin(),
                std::back_inserter(res),
                [](int a, int b){ return a+b; }
            );

            EXPECT_NE(res, just(0));
            EXPECT_EQ(res, just(3));
        }

        {
            auto arg1 = just(1);
            auto arg0 = nothing<int>();
            // this will not fail if res is empty
            // cos we use back_inserter
            auto res = nothing<int>();
            std::transform(
                arg0.begin(),
                arg0.end(),
                arg1.begin(),
                std::back_inserter(res),
                [](int a, int b){ return a+b; }
            );

            EXPECT_EQ(res, nothing());
        }
    }
}

TEST(Util, iterators) {
    {
        std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for(int a : view(iterate_keys(ints.begin()), iterate_keys(ints.end()))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        ASSERT_EQ(keys, pattern);
    }

    {
        const std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for(int a : view(iterate_keys(ints.begin()), iterate_keys(ints.end()))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        ASSERT_EQ(keys, pattern);
    }

    {
        std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for(const int& a : view(iterate_keys(begin_end_pair(ints)))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        ASSERT_EQ(keys, pattern);
    }

    {
        const std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for(const int& a : view(iterate_keys(begin_end_pair(ints)))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        ASSERT_EQ(keys, pattern);
    }


    {
        std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> values;

        for(int a : view(iterate_values(ints.begin()), iterate_values(ints.end()))) {
            values.push_back(a);
        }

        std::list<int> pattern { 1, 3, 5, 7 };
        ASSERT_EQ(values, pattern);
    }

    {
        const std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> values;

        for(int a : view(citerate_values(ints.begin()), citerate_values(ints.end()))) {
            values.push_back(a);
        }

        std::list<int> pattern { 1, 3, 5, 7 };
        ASSERT_EQ(values, pattern);
    }

    {
        std::vector<std::list<int>> con {
            { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }
        };

        std::vector<int> con2 { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> pat { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        std::copy(flat_iterator(con.begin(), con.end()), flat_iterator(con.end(), con.end()), con2.begin());

        ASSERT_EQ(con2, pat);
    }

    {
        std::vector<std::list<std::vector<int>>> con {
            { { 1, 2, 3 }, { 4, 5, 6 } }, { { 7 } }, { { 8, 9 } }
        };

        std::vector<int> con2 { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> pat { 1, 2, 3, 4, 5, 6, 7, 8, 9 };



        std::copy(
                flat2_iterator(con.begin(), con.end()),
                flat2_iterator(con.end()),
                con2.begin()
        );

        ASSERT_EQ(con2, pat);
    }
}

} // namespace _
