/*
 * AnnotatorPass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#include "AnnotatorPass.h"

namespace borealis {
	char AnnotatorPass::ID;
}

namespace {
static llvm::RegisterPass<borealis::AnnotatorPass>
X("annotator", "Anno annotation language processor", false, false);
}



