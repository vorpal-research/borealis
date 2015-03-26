/*
 * Executor.h
 *
 *  Created on: Mar 19, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_EXECUTOR_H_
#define EXECUTOR_EXECUTOR_H_

#include <unordered_map>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/CallSite.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Support/DataTypes.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/raw_ostream.h>

#include "Executor/ExecutionEngine.h"
#include "Executor/Arbiter.h"
#include "Codegen/intrinsics_manager.h"

namespace borealis {

class Executor {
    ExecutionEngine ee;

public:
    explicit Executor(
        llvm::Module *M,
        const llvm::DataLayout* TD,
        const llvm::TargetLibraryInfo* TLI,
        VariableInfoTracker* VIT,
        Arbiter::Ptr Aldaris);

    ~Executor();
};

} /* namespace borealis */

#endif /* EXECUTOR_EXECUTOR_H_ */
