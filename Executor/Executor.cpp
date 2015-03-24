/*
 * Executor.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: belyaev
 */

#include <Executor/Executor.h>

namespace borealis {

Executor::Executor(
    llvm::Module *M,
    const llvm::DataLayout* TD,
    const llvm::TargetLibraryInfo* TLI,
    Arbiter::Ptr Aldaris):
        ee{ M, TD, TLI, std::move(Aldaris) }{}

Executor::~Executor(){}

} /* namespace borealis */
