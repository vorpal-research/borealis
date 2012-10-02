/*
 * testmain.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */


#include "gtest/gtest.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

