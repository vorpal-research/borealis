/*
 * PassModularizer.hpp
 *
 *  Created on: Feb 8, 2013
 *      Author: belyaev
 */

#ifndef PASSMODULIZER_HPP_
#define PASSMODULIZER_HPP_

#include <llvm/Pass.h>

#include <unordered_map>
#include <utility>

#include "Passes/Util/SCCPass.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace impl_ {

template<class SubPass, bool Lazy>
class PassModularizerImpl : public SCCPass {

    typedef std::unique_ptr<SubPass> subptr;

    std::unordered_map<llvm::Function*, subptr> passes;
    subptr defaultPass;

    subptr createSubPass(llvm::Module& M) {
        auto res = util::uniq( new SubPass(this) );
        res->doInitialization(M);
        return std::move(res);
    }

public:

    static char ID;

    PassModularizerImpl() : SCCPass(ID), defaultPass(new SubPass(this)) {}

    virtual bool runOnSCC(CallGraphSCC& SCC) {
        using namespace llvm;

        if (Lazy) return false;

        bool changed = false;
        for (CallGraphSCCNode node : SCC) {
            Function* F = node->getFunction();
            // Do not run on declarations
            if (F && !F->isDeclaration()) {
                subptr ptr(createSubPass(*F->getParent()));
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
        } else if (Lazy) {
            subptr ptr(createSubPass(*F->getParent()));
            ptr->runOnFunction(*F);
            passes[F] = std::move(ptr);
            return *passes[F];
        } else {
            BYE_BYE(SubPass&, "Unknown function: " + F->getName().str());
        }
    }
};

template<class SubPass, bool Lazy>
char PassModularizerImpl<SubPass, Lazy>::ID;

} // namespace impl_

// Should not be used explicitly!
// Use ShouldBeModularized / ShouldBeLazyModularized marker traits
// from Util/passes.hpp instead.

template<class SubPass>
using PassModularizer = impl_::PassModularizerImpl<SubPass, false>;

template<class SubPass>
using LazyPassModularizer = impl_::PassModularizerImpl<SubPass, true>;

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PASSMODULIZER_HPP_ */
