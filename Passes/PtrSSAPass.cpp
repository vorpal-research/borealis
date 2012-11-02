/*
 * PtrSSAPass.cpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#include "PtrSSAPass.h"

char borealis::PtrSSAPass::ID;
static llvm::RegisterPass<borealis::PtrSSAPass>
X("pointer-ssa", "The pass that places an intrinsic mark on every pointer before every use");
