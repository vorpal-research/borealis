/*
 * ForceInline.cpp
 *
 *  Created on: Jan 27, 2014
 *      Author: belyaev
 */

#include <llvm/Analysis/InlineCost.h>

#include "Passes/Transform/ForceInline.h"
#include "Util/passes.hpp"

namespace borealis {

llvm::InlineCost ForceInline::getInlineCost(llvm::CallSite) {
    return llvm::InlineCost::getAlways();
}

char ForceInline::ID = 42;
static RegisterPass<ForceInline>
X("force-inline", "Pass that forces inlining of everything");

} /* namespace borealis */
