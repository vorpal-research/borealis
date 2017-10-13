#ifndef CALL_GRAPH_SLICER_H
#define CALL_GRAPH_SLICER_H

#include "Logging/logger.hpp"

#include <llvm/Pass.h>

#include <unordered_set>

namespace borealis {

class CallGraphSlicer:
    public llvm::ModulePass,
    public logging::ClassLevelLogging<CallGraphSlicer> {

    std::unordered_set<const llvm::Function*> slice;
    std::unordered_set<const llvm::Function*> addressTakenFunctions;

public:

    static char ID;


#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("cfg-slicer")
#include "Util/unmacros.h"

    CallGraphSlicer();

    const std::unordered_set<const llvm::Function*>& getSlice() const { return slice; }
    const std::unordered_set<const llvm::Function*>& getAddressTakenFunctions() const { return addressTakenFunctions; }
    bool doSlicing() const { return !slice.empty(); }

    virtual bool runOnModule(llvm::Module& M) override;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual ~CallGraphSlicer();
};

} /* namespace borealis */

#endif /* CALL_GRAPH_SLICER_H */

