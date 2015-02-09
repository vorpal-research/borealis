/*
 * test_util.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */

#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include "Util/iterators.hpp"
#include "Util/util.h"

namespace {

using namespace borealis;
using namespace borealis::util;
using namespace borealis::util::streams;

struct {
    template<class T>
    auto operator()(T&& i) -> typename std::remove_reference<T>::type {
        auto res = nothing<int>();
        return res.getOrElse(std::forward<T>(i));
    }
} forwardingGetOrElser;

TEST(Util, copy) {

	{
		struct fail_on_copy {
			int numcopies;
			fail_on_copy() : numcopies(0) {};
			fail_on_copy(const fail_on_copy& that) : numcopies(that.numcopies) {
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

		EXPECT_EQ(42, v1[0]);

		EXPECT_NE(v1[0], v2[0]);
		EXPECT_NE(v1, v2);
	}

}

TEST(Util, toString) {
    {
        EXPECT_EQ("23",   toString(23));
        EXPECT_EQ("true", toString(true));
        EXPECT_EQ("foo",  toString("foo"));

        void* ptr = reinterpret_cast<void*>(0xDEADBEEF);
        EXPECT_EQ("0xdeadbeef", toString(ptr));

        std::vector<int> vec{1,2,3,4};
        EXPECT_EQ("[1, 2, 3, 4]", toString(vec));

        std::set<int> set{1,2,3,4};
        EXPECT_EQ("(1, 2, 3, 4)", toString(set));
    }
}

TEST(Util, ltlt) {

//	{
//		std::string fill;
//		llvm::raw_string_ostream ost(fill);
//
//		std::vector<int> vec{1,2,3,4};
//		ost << vec;
//		EXPECT_EQ("[1, 2, 3, 4]", ost.str());
//	}

	{
		std::ostringstream ost;

		std::vector<int> vec{1,2,3,4};
		ost << vec;
		EXPECT_EQ("[1, 2, 3, 4]", ost.str());
	}

    {
        std::ostringstream ost;

        std::vector<const char*> vec{"one","two","three","four"};
        ost << vec;
        EXPECT_EQ("[one, two, three, four]", ost.str());
    }

}

TEST(Util, option) {

    {
        option<int> op = just(2);

        EXPECT_EQ(2, op.getUnsafe());
        EXPECT_FALSE(op.empty());

        int count = 0;
        for (auto& i : op) {
            count++;
            EXPECT_EQ(2, i);
        }
        EXPECT_EQ(1, count);
    }

    {
        option<int> nop = nothing();
        EXPECT_TRUE(nop.empty());

        for (auto& i : nop) {
            ADD_FAILURE() << "Got " << i << " from nothing!";
        }
    }

    {
        auto arg0 = just(1);
        auto arg1 = just(2);
        auto res = just(0);
        // this will fail if res is empty
        std::transform(
                arg0.begin(), arg0.end(), arg1.begin(), res.begin(),
                [](int a, int b){ return a+b; }
        );

        EXPECT_NE(just(0), res);
        EXPECT_EQ(just(3), res);
    }

    {
        auto arg0 = just(1);
        auto arg1 = just(2);
        auto res = nothing<int>();
        // this will not fail if res is empty
        // 'cause we use back_inserter
        std::transform(
            arg0.begin(), arg0.end(), arg1.begin(), std::back_inserter(res),
            [](int a, int b){ return a+b; }
        );

        EXPECT_NE(just(0), res);
        EXPECT_EQ(just(3), res);
    }

    {
        auto arg0 = nothing<int>();
        auto arg1 = just(1);
        auto res = nothing<int>();
        // this will not fail if arg0 is empty
        std::transform(
            arg0.begin(), arg0.end(), arg1.begin(), res.begin(),
            [](int a, int b){ return a+b; }
        );

        EXPECT_EQ(nothing<int>(), res);
    }

    {
        auto res = nothing<int>();
        int x = 42;
        auto xs = []{ return 23; };

        EXPECT_EQ(2,  res.getOrElse(2));
        EXPECT_EQ(42, res.getOrElse(x));
        EXPECT_EQ(23, res.getOrElse(xs()));

        EXPECT_EQ(2,  forwardingGetOrElser(2));
        EXPECT_EQ(42, forwardingGetOrElser(x));
        EXPECT_EQ(23, forwardingGetOrElser(xs()));
    }

} // TEST(Util, option)

TEST(Util, iterators) {

    {
        std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for (auto& a : view(iterate_keys(ints.begin()), iterate_keys(ints.end()))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        EXPECT_EQ(pattern, keys);
    }

    {
        const std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for (auto& a : view(iterate_keys(ints.begin()), iterate_keys(ints.end()))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        EXPECT_EQ(pattern, keys);
    }

    {
        std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for (auto& a : view(iterate_keys(begin_end_pair(ints)))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        EXPECT_EQ(pattern, keys);
    }

    {
        const std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> keys;

        for (auto& a : view(iterate_keys(begin_end_pair(ints)))) {
            keys.push_back(a);
        }

        std::list<int> pattern { 0, 2, 4, 6 };
        EXPECT_EQ(pattern, keys);
    }

    {
        std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> values;

        for (auto& a : view(iterate_values(ints.begin()), iterate_values(ints.end()))) {
            values.push_back(a);
        }

        std::list<int> pattern { 1, 3, 5, 7 };
        EXPECT_EQ(pattern, values);
    }

    {
        const std::map<int, int> ints{
            { 0, 1 },
            { 2, 3 },
            { 4, 5 },
            { 6, 7 }
        };
        std::list<int> values;

        for (auto& a : view(citerate_values(ints.begin()), citerate_values(ints.end()))) {
            values.push_back(a);
        }

        std::list<int> pattern { 1, 3, 5, 7 };
        EXPECT_EQ(pattern, values);
    }

    {
        std::vector<std::list<int>> con {
            { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }
        };

        std::vector<int> con2 { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> pat  { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        std::copy(
            flat_iterator(con.begin(), con.end()),
            flat_iterator(con.end()),
            con2.begin()
        );

        EXPECT_EQ(pat, con2);
    }

    {
        std::vector<std::list<int>> con {
            { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }
        };

        std::vector<int> pat  { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        EXPECT_EQ(pat, viewContainer(con).flatten().to<std::vector<int>>());
    }

    {
        std::vector<std::list<std::vector<int>>> con {
            { { 1, 2, 3 }, { 4, 5, 6 } }, { { 7 } }, { { 8, 9 } }
        };

        std::vector<int> con2 { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        std::vector<int> pat  { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        std::copy(
            flat2_iterator(con.begin(), con.end()),
            flat2_iterator(con.end()),
            con2.begin()
        );

        EXPECT_EQ(pat, con2);
    }

    {
        std::vector<int> con {
            1, 2, 3, 4, 5, 6, 7, 8, 9
        };

        std::vector<int> con2 { 0, 0, 0, 0 };
        std::vector<int> pat  { 2, 4, 6, 8 };

        auto is_even = [](int v){ return v % 2 == 0; };

        std::copy(
            filter_iterator(con.begin(), con.end(), is_even),
            filter_iterator(con.end(), is_even),
            con2.begin()
        );

        EXPECT_EQ(pat, con2);
    }

    {
        std::vector<int> con0 {
            1, 2, 3, 4, 5, 6, 7, 8, 9
        };

        std::vector<int> con1 { 10, 11, 12, 13 };
        std::vector<int> con(glue_iterator(
                std::make_pair(con0.begin(), con0.end()),
                std::make_pair(con1.begin(), con1.end())
        ), glued_iterator<typename std::vector<int>::iterator>());
        std::vector<int> pat  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

        EXPECT_EQ(pat, con);
    }

} // TEST(Util, iterators)

TEST(Util, tuple) {
    {
        std::tuple<int, float, double> tuple(42, 3.17, 14.88);

        std::tuple<float, double> cdr = cdr_tuple(tuple);

        EXPECT_FLOAT_EQ(3.17, std::get<0>(cdr));
        EXPECT_DOUBLE_EQ(14.88, std::get<1>(cdr));
    }
}

TEST(Util, reduce) {
    {
        auto&& plus = [](auto&& a, auto&& b) { return a + b; };

        std::vector<int> vec{};
        EXPECT_EQ(nothing(), viewContainer(vec).reduce(plus));
        vec.push_back(1);
        EXPECT_EQ(just(1), viewContainer(vec).reduce(plus));
        vec.push_back(1000);
        EXPECT_EQ(just(1001), viewContainer(vec).reduce(plus));
    }
}

} // namespace
