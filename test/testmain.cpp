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

  borealis::config::Config cfg{
      new borealis::config::FileConfigSource("wrapper.tests.conf")
  };
  auto logFile = cfg.getValue<std::string>("logging", "ini");
  for (const auto& op : logFile) {
      borealis::logging::configureLoggingFacility(op);
  }

  return RUN_ALL_TESTS();
}
