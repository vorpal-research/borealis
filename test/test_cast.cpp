/*
 * test_cast.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>

#include "Util/cast.hpp"

#include "Factory/Nest.h"
#include "Logging/logger.hpp"

namespace {

using namespace borealis;
using namespace borealis::logging;
using namespace borealis::util;

TEST(Cast, visit) {
    {
        using borealis::util::visit;

        auto TF = FactoryNest().Term;
        auto check = false;

        auto trm = TF->getOpaqueConstantTerm(true);
        visit(*trm)
            .oncase<OpaqueIntConstantTerm>([&](const OpaqueIntConstantTerm&){
                check = false;
            })
            .oncase<OpaqueBoolConstantTerm>([&](const OpaqueBoolConstantTerm& b){
                check = b.getValue();
            });
        EXPECT_TRUE(check);

        auto trm2 = TF->getOpaqueConstantTerm(false);
        check = visit(*trm2)
                    .on<OpaqueBoolConstantTerm>([&](const OpaqueBoolConstantTerm& b){
                        return b.getValue();
                    })
                    .getOrElse(true);

        EXPECT_FALSE(check);
    }
}

TEST(Cast, pair_matcher) {
    {
        auto TF = FactoryNest().Term;
        auto lhv = TF->getOpaqueConstantTerm(true);
        auto rhv = TF->getOpaqueConstantTerm(0xC0DEBEEF, 0x0);

        if (auto matched = match_pair<OpaqueBoolConstantTerm, OpaqueIntConstantTerm>(lhv, rhv)) {
            EXPECT_EQ(true, matched->first->getValue());
            EXPECT_EQ(0xC0DEBEEFLL, matched->second->getValue());
        } else {
            FAIL() << "match_pair didn't match valid elements";
        }

        if (match_pair<OpaqueIntConstantTerm, OpaqueIntConstantTerm>(lhv, rhv)) {
            FAIL() << "match_pair matched invalid elements";
        }
    }
}


TEST(Cast, tuple_matcher) {
    {
        auto TF = FactoryNest().Term;
        auto lhv = TF->getOpaqueConstantTerm(true);
        auto rhv = TF->getOpaqueConstantTerm(0xC0DEBEEF, 0x0);

        if (auto matched = match_tuple<OpaqueBoolConstantTerm, OpaqueIntConstantTerm>::doit(lhv, rhv)) {
            EXPECT_EQ(true, matched->get<0>()->getValue());
            EXPECT_EQ(0xC0DEBEEFLL, matched->get<1>()->getValue());
        } else {
            FAIL() << "match_tuple didn't match valid elements";
        }

        if (match_tuple<OpaqueIntConstantTerm, OpaqueIntConstantTerm>::doit(lhv, rhv)) {
            FAIL() << "match_tuple matched invalid elements";
        }
    }
}

} // namespace
