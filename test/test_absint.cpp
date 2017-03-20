//
// Created by abdullin on 3/15/17.
//

#include <gtest/gtest.h>

#include "Interpreter/Domain/DomainFactory.h"

namespace {

TEST(AbstractInterpreter, IntegerInterval) {

    using namespace borealis::absint;

    auto factory = DomainFactory();

    auto int1 = factory.getInteger(32, false);
    auto int2 = factory.getInteger(32, false);
    auto int3 = factory.getInteger(llvm::APInt(32, 0, false), llvm::APInt(32, 10, false));
    auto int4 = factory.getInteger(llvm::APInt(32, 1, false), llvm::APInt(32, 11, false));
    auto int5 = factory.getInteger(llvm::APInt(32, 0, false), llvm::APInt(32, 11, false));

    ASSERT_EQ(int1, int2);
    ASSERT_EQ(int3->join(int4), int5);
    ASSERT_EQ(int3->widen(int4)->isTop(), true);
    ASSERT_EQ(int4->widen(int3), int5);
}

TEST(AbstractInterpreter, FloatInterval) {

    using namespace borealis::absint;

    auto factory = DomainFactory();


    auto& semantics = llvm::APFloat::IEEEdouble;
    auto float1 = factory.getFloat(semantics);
    auto float2 = factory.getFloat(llvm::APFloat(double(0.0)), llvm::APFloat(double(10.0)));
    auto float3 = factory.getFloat(llvm::APFloat(double(1.0)), llvm::APFloat(double(11.0)));
    auto float4 = factory.getFloat(llvm::APFloat(double(0.0)), llvm::APFloat(double(11.0)));
    auto float5 = factory.getFloat(llvm::APFloat(double(0.0)), borealis::util::getMaxValue(semantics));
    auto float6 = factory.getFloat(borealis::util::getMinValue(semantics), llvm::APFloat(double(11.0)));

    ASSERT_EQ(float1, factory.getFloat(Domain::BOTTOM, semantics));
    ASSERT_EQ(float2->join(float3), float4);
    ASSERT_EQ(float2->widen(float3), float5);
    ASSERT_EQ(float3->widen(float2), float6);
}

TEST(AbstractInterpreter, Pointer) {

    using namespace borealis::absint;

    auto factory = DomainFactory();

    auto bottom = factory.getPointer();
    auto valid = factory.getPointer(true);
    auto nonvalid = factory.getPointer(false);
    auto top = factory.getPointer(Domain::TOP);

    ASSERT_EQ(bottom, factory.getPointer());
    ASSERT_EQ(valid->join(nonvalid), top);
    ASSERT_EQ(valid->widen(nonvalid), top);
}

}   // namespace