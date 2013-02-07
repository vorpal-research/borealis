/*
 * test_cast.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: kopcap
 */

#include <gtest/gtest.h>
#include <z3/z3++.h>

#include "Util/cast.hpp"

#include "Logging/logger.hpp"
#include "Term/TermFactory.h"

namespace {

using namespace borealis;
using namespace borealis::logging;
using namespace borealis::util;

static stream_t infos() {
    return infosFor("test");
}

TEST(cast, visit) {
    {
        using borealis::util::visit;
        using borealis::Term;
        using borealis::OpaqueBoolConstantTerm;
        using borealis::OpaqueIntConstantTerm;

        auto tf = TermFactory::get(nullptr);
        auto trm = tf->getOpaqueConstantTerm(true);


        auto check = false;
        visit(*trm)
            .oncase<OpaqueIntConstantTerm>([&](const OpaqueIntConstantTerm&){
                check = false;
            })
            .oncase<OpaqueBoolConstantTerm>([&](const OpaqueBoolConstantTerm& b){
                check = b.getValue();
            });

        ASSERT_TRUE(check);

        auto trm2 = tf->getOpaqueConstantTerm(false);
        auto fls = false;

        check = visit(*trm2).on<OpaqueBoolConstantTerm>([&](const OpaqueBoolConstantTerm& b){
            return b.getValue();
        }).getOrElse(fls);

        ASSERT_FALSE(check);

    }
}

} /* namespace */
