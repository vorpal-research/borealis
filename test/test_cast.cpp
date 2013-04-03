/*
 * test_cast.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>

#include "Util/cast.hpp"

#include "Logging/logger.hpp"
#include "Term/TermFactory.h"

namespace {

using namespace borealis;
using namespace borealis::logging;
using namespace borealis::util;

TEST(Cast, visit) {
    {
        using borealis::util::visit;

        auto tf = TermFactory::get(nullptr);
        auto check = false;

        auto trm = tf->getOpaqueConstantTerm(true);
        visit(*trm)
            .oncase<OpaqueIntConstantTerm>([&](const OpaqueIntConstantTerm&){
                check = false;
            })
            .oncase<OpaqueBoolConstantTerm>([&](const OpaqueBoolConstantTerm& b){
                check = b.getValue();
            });
        ASSERT_TRUE(check);

        auto trm2 = tf->getOpaqueConstantTerm(false);
        check = visit(*trm2)
                    .on<OpaqueBoolConstantTerm>([&](const OpaqueBoolConstantTerm& b){
                        return b.getValue();
                    })
                    .getOrElse(true);

        ASSERT_FALSE(check);
    }
}

TEST(Cast, pair_matcher) {
    {
        auto tf = TermFactory::get(nullptr);
        auto lhv = tf->getOpaqueConstantTerm(true);
        auto rhv = tf->getOpaqueConstantTerm(0xC0DEBEEFLL);

        if (auto matched = match_pair<OpaqueBoolConstantTerm, OpaqueIntConstantTerm>(lhv, rhv)) {
            ASSERT_EQ(true, matched->first->getValue());
            ASSERT_EQ(0xC0DEBEEFLL, matched->second->getValue());
        } else {
            FAIL();
        }

        if (auto matched = match_pair<OpaqueIntConstantTerm, OpaqueIntConstantTerm>(lhv, rhv)) {
            FAIL();
        }
    }
}

/*******************************************************************************
TEST(Cast, tuple_matcher) {
    {
        auto tf = TermFactory::get(nullptr);
        auto lhv = tf->getOpaqueConstantTerm(true);
        auto rhv = tf->getOpaqueConstantTerm(0xC0DEBEEFLL);

        if (auto matched = match_tuple<OpaqueBoolConstantTerm, OpaqueIntConstantTerm>::doit(lhv, rhv)) {
            ASSERT_EQ(true, matched.get<0>()->getValue());
            ASSERT_EQ(0xC0DEBEEFLL, matched.get<1>()->getValue());
        } else {
            FAIL();
        }

        if (auto matched = match_tuple<OpaqueIntConstantTerm, OpaqueIntConstantTerm>::doit(lhv, rhv)) {
            FAIL();
        }
    }
}
*******************************************************************************/

} // namespace
