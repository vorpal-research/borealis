/*
 * PtrSSAPass.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "Passes/Transform/PtrSSAPass.h"
#include "Util/passes.hpp"

namespace borealis {

char PtrSSAPass::ID;
static RegisterPass<PtrSSAPass>
X("pointer-ssa", "Pass that places an intrinsic mark on every pointer before every use");

} // namespace borealis
