/*
 * test_protobuf.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include "Factory/Nest.h"
#include "Protobuf/Converter.hpp"

namespace {

TEST(Protobuf, protobuffy) {

    using namespace borealis;

    {
        auto FN = FactoryNest(nullptr);
        auto TF = FN.Type;

        auto type = TF->getPointer(
            TF->getPointer(
                TF->getFloat()
            )
        );

        EXPECT_EQ(type, deprotobuffy(FN, *protobuffy(type)));
    }

}

} // namespace
