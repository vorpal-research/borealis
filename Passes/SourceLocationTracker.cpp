/*
 * SourceLocationTracker.cpp
 *
 *  Created on: Oct 4, 2012
 *      Author: belyaev
 */


#include "SourceLocationTracker.h"

char SourceLocationTracker::ID;
static llvm::RegisterPass<SourceLocationTracker>
X("source-location-tracker", "Clang metadata extracter to provide values with their source locations");
