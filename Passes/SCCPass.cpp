/*
 * SCCPass.cpp
 *
 *  Created on: Feb 7, 2013
 *      Author: ice-phoenix
 */

#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/GlobalValue.h>
#include <llvm/Support/raw_ostream.h>

#include "Passes/CheckNullDereferencePass.h"
#include "Passes/SCCPass.h"

#include "Logging/logger.hpp"
#include "Util/util.h"

namespace borealis {

SCCPass::SCCPass() : llvm::ModulePass(ID) {}

SCCPass::~SCCPass() {}

void SCCPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    using namespace llvm;

    AU.setPreservesAll();
    AU.addRequiredTransitive<CallGraph>();

    AU.addRequiredTransitive<CheckNullDereferencePass::MX>();
}

bool SCCPass::runOnModule(llvm::Module&) {

    using namespace llvm;

    auto* CG = &getAnalysis<CallGraph>();
    for (auto& SCC : borealis::util::view(scc_begin(CG), scc_end(CG))) {
        for (auto* e : SCC)
            infos() << *e->getFunction() << endl;
    }

    return false;
}

char SCCPass::ID;
static llvm::RegisterPass<SCCPass>
X("scc", "SCC test pass");

} /* namespace borealis */
