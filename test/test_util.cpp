/*
 * test_util.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */

#include <vector>
#include <utility>

#include <gtest/gtest.h>

#include <unordered_set>

#include <debugbreak/debugbreak.h>

#include "Util/iterators.hpp"
#include "Util/util.h"
#include "Util/hash.hpp"
#include "Util/hamt.hpp"


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

        for (const int& a : viewContainerKeys(ints)) {
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

        for (const int& a : viewContainerKeys(ints)) {
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

        for (int& a : viewContainerValues(ints)) {
            values.push_back(a);
        }

        std::list<int> pattern { 1, 3, 5, 7 };
        EXPECT_EQ(pattern, values);
    }


    {
        std::vector<std::list<int>> con {
            { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }
        };

        std::vector<int> pat  { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        std::vector<int> con2 = viewContainer(con).flatten().toVector();

        EXPECT_EQ(pat, con2);
    }

    {
        std::vector<std::list<std::vector<int>>> con {
            { { 1, 2, 3 }, { 4, 5, 6 } }, { { 7 } }, { { 8, 9 } }
        };

        std::vector<int> pat  { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        std::vector<int> con2 = viewContainer(con).flatten().flatten().toVector();

        EXPECT_EQ(pat, con2);
    }

    {
        std::vector<int> con {
            1, 2, 3, 4, 5, 6, 7, 8, 9
        };

        std::vector<int> pat  { 2, 4, 6, 8 };

        auto is_even = [](int v){ return v % 2 == 0; };

        auto con2 = viewContainer(con).filter(is_even).toVector();

        EXPECT_EQ(pat, con2);
    }

    {
        std::vector<int> con0 {
            1, 2, 3, 4, 5, 6, 7, 8, 9
        };

        std::vector<int> con1 { 10, 11, 12, 13 };
        std::vector<int> con = (viewContainer(con0) >> viewContainer(con1)).toVector();
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
        vec.push_back(1);
        EXPECT_EQ(1, viewContainer(vec).reduce(0, plus));
        vec.push_back(1000);
        EXPECT_EQ(1001, viewContainer(vec).reduce(0, plus));
    }
}

} // namespace

#include "Util/generate_macros.h"

class example_struct {
    int x;
    std::string y;
public:
    friend struct std::hash<example_struct>;
    friend struct borealis::util::json_traits<example_struct>;

    GENERATE_CONSTRUCTOR(example_struct, x, y);
    GENERATE_COPY_CONSTRUCTOR(example_struct, x, y);
    GENERATE_MOVE_CONSTRUCTOR(example_struct, x, y);
    GENERATE_ASSIGN(example_struct, x, y);
    GENERATE_MOVE_ASSIGN(example_struct, x, y);
    GENERATE_EQ(example_struct, x, y);
    GENERATE_LESS(example_struct, x, y);
};

GENERATE_OUTLINE_HASH(example_struct, x, y);
GENERATE_OUTLINE_JSON_TRAITS(example_struct, x, y);

#define JSON(...) #__VA_ARGS__

TEST(Util, generation_macros) {
    {
        example_struct es(42, "hello");
        example_struct es2 = es;
        example_struct es3 = std::move(es2);
        es2 = es3;
        EXPECT_EQ(es, es2);
        EXPECT_FALSE(es < es2);
        EXPECT_EQ(borealis::util::hash::simple_hash_value(es), borealis::util::hash::simple_hash_value(es2));
        std::istringstream ist(JSON({"x": 13, "y": "foo"}));

        auto obj = read_as_json<example_struct>(ist);
        EXPECT_TRUE(!!obj);
        EXPECT_EQ(*obj, example_struct(13, "foo"));
    }
}

TEST(Util, indexed_string) {
    {
        indexed_string is0 = "Hello";
        std::string tmp = "Hello";
        indexed_string is1 = tmp;

        EXPECT_EQ(is0.hash(), is1.hash());

        indexed_string is2 = is0;
        EXPECT_EQ(is2.hash(), is1.hash());

        EXPECT_EQ(is0.c_str(), is1.c_str());
        EXPECT_EQ(is0.str(), is1.str());
    }
}

struct hash_colliding {
    size_t value;
    hash_colliding(size_t value): value(value) {}

    friend bool operator==(const hash_colliding& l, const hash_colliding& r) { return l.value == r.value; }
    friend std::ostream& operator<< (std::ostream& ost, const hash_colliding& hc) {
        return ost << "hc<" << hc.value << ">";
    }
};

namespace std {

template<>
struct hash<hash_colliding> {
    size_t operator()(const hash_colliding&) const noexcept {
        return 54; // sic!
    }
};

} /* namespace std */

TEST(Util, hamt) {
    {
        hamt_set<size_t> s;

        auto s0 = s.insert(2);
        ASSERT_EQ(viewContainer(s0).toHashSet(), (std::unordered_set<size_t>{2}));
        auto s1 = s0.insert(4);
        ASSERT_EQ(viewContainer(s1).toHashSet(), (std::unordered_set<size_t>{2, 4}));
        auto s2 = s1.insert(666);
        ASSERT_EQ(viewContainer(s2).toHashSet(), (std::unordered_set<size_t>{2, 4, 666}));
        auto s3 = s2.insert(20);
        ASSERT_EQ(viewContainer(s3).toHashSet(), (std::unordered_set<size_t>{2, 4, 666, 20}));
        auto s4 = s3.insert(2);
        ASSERT_EQ(viewContainer(s4).toHashSet(), (std::unordered_set<size_t>{2, 4, 666, 20}));
        auto s5 = s4.insert(4);
        ASSERT_EQ(viewContainer(s5).toHashSet(), (std::unordered_set<size_t>{2, 4, 666, 20}));
        auto s6 = s5.insert(13);
        ASSERT_EQ(viewContainer(s6).toHashSet(), (std::unordered_set<size_t>{2, 4, 666, 20, 13}));
        auto s7 = s6.insert(84);
        ASSERT_EQ(viewContainer(s7).toHashSet(), (std::unordered_set<size_t>{2, 4, 666, 20, 13, 84}));
        auto s8 = s7.erase(4);
        ASSERT_EQ(viewContainer(s8).toHashSet(), (std::unordered_set<size_t>{2, 666, 20, 13, 84}));
        auto s9 = s8.erase(18);
        ASSERT_EQ(viewContainer(s9).toHashSet(), (std::unordered_set<size_t>{2, 666, 20, 13, 84}));
        auto s10 = s9.erase(666);
        ASSERT_EQ(viewContainer(s10).toHashSet(), (std::unordered_set<size_t>{2, 20, 13, 84}));
        auto s11 = s10.insert(~size_t(0));
        ASSERT_EQ(viewContainer(s11).toHashSet(), (std::unordered_set<size_t>{2, 20, 13, 84, ~size_t(0)}));

        hamt_set<hash_colliding> hc;
        auto hc2 = hc.insert(1).insert(2).insert(3).insert(4).insert(5).erase(3);
        ASSERT_EQ(viewContainer(hc2).toHashSet(), (std::unordered_set<hash_colliding>{1, 2, 4, 5}));

    }
}

#include "Util/generate_unmacros.h"


