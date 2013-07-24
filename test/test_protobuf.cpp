/*
 * test_protobuf.cpp
 *
 *  Created on: Jul 24, 2013
 *      Author: ice-phoenix
 */

#include <gtest/gtest.h>

#include "Protobuf/Converter.hpp"
#include "Type/TypeFactory.h"

namespace {

TEST(Protobuf, protobuffy) {

    using namespace borealis;

    {
        auto& tf = TypeFactory::getInstance();

        auto type = tf.getPointer(
            tf.getPointer(
                tf.getFloat()
            )
        );

        auto protobuf = protobuffy(type);

        EXPECT_NE(protobuf, nullptr);
    }

}

} // namespace
