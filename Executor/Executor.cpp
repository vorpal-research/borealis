/*
 * Executor.cpp
 *
 *  Created on: Jan 27, 2015
 *      Author: belyaev
 */

#include "Executor.h"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "Util/macros.h"
namespace borealis {


Executor::Executor(llvm::Module *M, llvm::DataLayout* TD): TD(TD)
{
    // TODO Auto-generated constructor stub

}

Executor::~Executor()
{
    // TODO Auto-generated destructor stub
}

} /* namespace borealis */
#include "Util/unmacros.h"
