/*
 * ProxyFunctionPass.cpp
 *
 *  Created on: Oct 24, 2012
 *      Author: belyaev
 */

#include "Passes/Util/ProxyFunctionPass.h"

namespace borealis {

ProxyFunctionPass::ProxyFunctionPass(char& ID, Pass* delegate) :
        llvm::FunctionPass(ID), delegate(delegate) {};

ProxyFunctionPass::~ProxyFunctionPass() {};

} // namespace borealis
