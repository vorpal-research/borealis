/*
 * SCCPass.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: ice-phoenix
 */

#include <llvm/ADT/SCCIterator.h>

#include "Passes/CheckNullDereferencePass.h"
#include "Passes/SCCPass.h"

#include "Logging/logger.hpp"
#include "Util/util.h"

namespace borealis {

SCCPass::SCCPass(char& ID) : llvm::ModulePass(ID) {}

SCCPass::~SCCPass() {}

void SCCPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AUX<llvm::CallGraph>::addRequiredTransitive(AU);
}

bool SCCPass::runOnModule(llvm::Module&) {
    using namespace llvm;

    bool changed = false;

    CallGraph* CG = &GetAnalysis<CallGraph>::doit(this);
    for (CallGraphSCC& SCC : borealis::util::view(scc_begin(CG), scc_end(CG))) {
        changed |= runOnSCC(SCC);
    }

    return changed;
}

////////////////////////////////////////////////////////////////////////////////

class SCCTestPass : public SCCPass {

public:

    static char ID;

    SCCTestPass() : SCCPass(ID) {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        SCCPass::getAnalysisUsage(AU);
        AU.setPreservesAll();
        AUX<CheckNullDereferencePass>::addRequiredTransitive(AU);
    }

    virtual bool runOnSCC(CallGraphSCC& SCC) {
        using namespace llvm;

        for (CallGraphSCCNode node : SCC) {
            Function* F = node->getFunction();
            if (F) {
                infos() << "SCC: " << F->getName() << endl;
            }
        }
        return false;
    }
};

char SCCTestPass::ID;
static RegisterPass<SCCTestPass>
X("scc-test", "Test pass for SCCPass");

} /* namespace borealis */
