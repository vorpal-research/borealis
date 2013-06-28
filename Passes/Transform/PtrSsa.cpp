/*
 * PtrSSAPass.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "Passes/Transform/PtrSsa.h"
#include "Util/passes.hpp"

namespace borealis {

char PtrSsa::ID;
static RegisterPass<PtrSsa>
X("pointer-ssa", "Pass that places an intrinsic mark on every pointer before every use");

} // namespace borealis
