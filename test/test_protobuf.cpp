/*
 * test_protobuf.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include <math.h>

#include "Factory/Nest.h"
#include "Protobuf/Converter.hpp"

namespace {

TEST(Protobuf, protobuffy) {

    using namespace borealis;

    {
        auto FN = FactoryNest(nullptr);
        auto TyF = FN.Type;

        auto type = TyF->getPointer(
            TyF->getPointer(
                TyF->getFloat()
            )
        );

        EXPECT_EQ(type, deprotobuffy(FN, *protobuffy(type)));
    }

    {
        auto FN = FactoryNest(nullptr);
        auto PF = FN.Predicate;
        auto TF = FN.Term;

        auto t1 = PF->getEqualityPredicate(
            TF->getBooleanTerm(true),
            TF->getBooleanTerm(false)
        );

        EXPECT_EQ(*t1, *deprotobuffy(FN, *protobuffy(t1)));
        EXPECT_NE( t1,  deprotobuffy(FN, *protobuffy(t1)));

        auto t2 = PF->getEqualityPredicate(
            TF->getBooleanTerm(true),
            TF->getCmpTerm(
                llvm::ConditionType::NEQ,
                TF->getBooleanTerm(false),
                TF->getBinaryTerm(
                    llvm::ArithType::XOR,
                    TF->getIntTerm(0xDEADBEEF),
                    TF->getRealTerm(M_PI)
                )
            )
        );

        EXPECT_EQ(*t2, *deprotobuffy(FN, *protobuffy(t2)));
        EXPECT_NE( t2,  deprotobuffy(FN, *protobuffy(t2)));
    }

}

} // namespace
