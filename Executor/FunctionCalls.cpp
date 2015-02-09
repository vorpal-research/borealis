/*
 * FunctionCalls.cpp
 *
 *  Created on: Feb 5, 2015
 *      Author: belyaev
 */
#include "Executor/Executor.h"

using namespace borealis;

#include <Logging/tracer.hpp>

llvm::GenericValue Executor::callExternalFunction(
    llvm::Function *F,
    const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    // TODO
    return llvm::GenericValue{};
}


