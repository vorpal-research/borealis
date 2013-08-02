/*
 * testmain.cpp
 *
 *  Created on: Oct 2, 2012
 *      Author: belyaev
 */

#include <google/protobuf/stubs/common.h>
#include <gtest/gtest.h>

#include "Config/config.h"

int main(int argc, char** argv) {

  GOOGLE_PROTOBUF_VERIFY_VERSION;
  atexit(google::protobuf::ShutdownProtobufLibrary);

  ::testing::InitGoogleTest(&argc, argv);

  borealis::config::Config cfg("wrapper.conf");
  auto logCFG = cfg.getValue<std::string>("logging", "ini");
  if (!logCFG.empty()) {
      borealis::logging::configureLoggingFacility(*logCFG.get());
  }

  return RUN_ALL_TESTS();
}
