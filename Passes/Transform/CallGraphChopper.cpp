//
// Created by belyaev on 10/27/16.
//

#include <Util/functional.hpp>
#include "Util/collections.hpp"
#include "Passes/Transform/CallGraphChopper.h"
#include "Passes/Checker/CallGraphSlicer.h"

#include "Util/passes.hpp"

namespace borealis {

void CallGraphChopper::getAnalysisUsage(llvm::AnalysisUsage& AU) const{
    AUX<CallGraphSlicer>::addRequired(AU);
}

bool CallGraphChopper::runOnModule(llvm::Module& M) {
    auto&& cgs = getAnalysis<CallGraphSlicer>();

    if(not cgs.doSlicing()) return false;

    auto&& slice = cgs.getSlice();
    auto&& addressTaken = cgs.getAddressTakenFunctions();

    auto allFuncs = util::viewContainer(M).map(ops::take_pointer).filter().toVector();

    for(llvm::Function* F : allFuncs) {
        if(not slice.count(F) && not addressTaken.count(F) && not F->isDeclaration()) {
            F->deleteBody();
            if(not F->hasNUsesOrMore(1)) F->eraseFromParent();
        }
    }

    return true;
}

void CallGraphChopper::print(llvm::raw_ostream &O, const llvm::Module*) const {
    O << "callgraph-chopper";
}

char CallGraphChopper::ID;
static RegisterPass<CallGraphChopper> X("callgraph-chopper", "Pass that removes bodies of unused functions");

} /* namespace borealis */

