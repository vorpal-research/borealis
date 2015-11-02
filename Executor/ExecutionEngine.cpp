/*
 * Executor.cpp
 *
 *  Created on: Jan 27, 2015
 *      Author: belyaev
 */

#include "Executor/ExecutionEngine.h"

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include <vector>

#include "Util/util.h"


#include "Logging/tracer.hpp"

#include "Util/macros.h"
namespace borealis {


ExecutionEngine::ExecutionEngine(
    llvm::Module* /* M */,
    const llvm::DataLayout* TD,
    const llvm::TargetLibraryInfo* TLI,
    const SlotTrackerPass* ST,
    VariableInfoTracker* VIT,
    Arbiter::Ptr Aldaris):
        ExitValue{}, TD{TD}, TLI{TLI}, ST{ST}, VIT{VIT}, IM{}, Judicator{ Aldaris },
        ECStack{}, AtExitHandlers{}, Mem{ *TD }, FNCache{}, IE{this}
{
    IM = &IntrinsicsManager::getInstance();
    FNCache = [ST](auto f){ return FactoryNest(f->getDataLayout(), ST->getSlotTracker(f)); };
}

ExecutionEngine::~ExecutionEngine()
{}


void ExecutionEngine::runAtExitHandlers () {
    while (!AtExitHandlers.empty()) {
        callFunction(AtExitHandlers.back(), std::vector<llvm::GenericValue>());
        AtExitHandlers.pop_back();
        run();
    }
}

/// run - Start execution with the specified function and arguments.
///
llvm::GenericValue
ExecutionEngine::runFunction(llvm::Function *F,
    const std::vector<llvm::GenericValue> &ArgValues) {
    TRACE_FUNC;

    ASSERTC (F && "Function *F was null at entry to run()");

    // Try extra hard not to pass extra args to a function that isn't
    // expecting them.  C programmers frequently bend the rules and
    // declare main() with fewer parameters than it actually gets
    // passed, and the interpreter barfs if you pass a function more
    // parameters than it is declared to take. This does not attempt to
    // take into account gratuitous differences in declared types,
    // though.
    std::vector<llvm::GenericValue> ActualArgs;
    const unsigned ArgCount = F->getFunctionType()->getNumParams();
    const auto realArgCount = ArgValues.size();

    for (unsigned i = 0; i < ArgCount; ++i)
        ActualArgs.push_back(i < realArgCount ? ArgValues[i] : llvm::GenericValue{});

    // Set up the function call.
    callFunction(F, ActualArgs);

    // Start executing the function.
    run();

    return ExitValue;
}

}; /* namespace borealis */
#include "Util/unmacros.h"
