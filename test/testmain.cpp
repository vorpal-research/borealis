/*
 * testmain.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */

#include "gtest/gtest.h"

#include "Config/config.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  borealis::config::Config cfg("wrapper.conf");
  auto logCFG = cfg.getValue<std::string>("logging", "ini");
  if (!logCFG.empty()) {
      borealis::logging::configureLoggingFacility(*logCFG.get());
  }

  return RUN_ALL_TESTS();
}
