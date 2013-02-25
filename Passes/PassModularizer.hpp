/*
 * PassModularizer.hpp
 *
 *  Created on: Feb 8, 2013
 *      Author: belyaev
 */

#ifndef PASSMODULIZER_HPP_
#define PASSMODULIZER_HPP_

#include <llvm/Module.h>
#include <llvm/Pass.h>
#include <llvm/PassAnalysisSupport.h>

#include <unordered_map>
#include <utility>

#include "Passes/SCCPass.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

// Should not be used explicitly
// Use ShouldBeModularized marker trait from Util/passes.hpp instead
//
template<class SubPass>
class PassModularizer : public SCCPass {

    typedef std::unique_ptr<SubPass> subptr;

    std::unordered_map<llvm::Function*, subptr> passes;
    subptr defaultPass;

    subptr createSubPass() {
        return subptr(new SubPass(this));
    }

public:

    static char ID;

    PassModularizer() : SCCPass(ID), defaultPass(new SubPass(this)) {}

    virtual bool runOnSCC(CallGraphSCC& SCC) {
        using namespace llvm;

        bool changed = false;
        for (CallGraphSCCNode node : SCC) {
            Function* F = node->getFunction();
            // Do not run on declarations
            if (F && !F->isDeclaration()) {
                subptr ptr(createSubPass());
                changed |= ptr->runOnFunction(*F);
                passes[F] = std::move(ptr);
            }
        }
        return changed;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        SCCPass::getAnalysisUsage(AU);
        defaultPass->getAnalysisUsage(AU);
    }

    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const {
        for (auto& p : passes) {
            p.second->print(O, M);
        }
    }

    SubPass& getResultsForFunction(llvm::Function* F) {
        if (passes.count(F) > 0) {
            return *passes[F];
        } else {
            BYE_BYE(SubPass&, "Unknown function: " + F->getName().str());
        }
    }
};

template<class SubPass>
char PassModularizer<SubPass>::ID;

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PASSMODULIZER_HPP_ */
