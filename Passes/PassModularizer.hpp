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

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

// Should be used only with [SubPass = ProxyFunctionPass<...>]
//
template<class SubPass>
class PassModularizer : public llvm::ModulePass {

    typedef std::unique_ptr<SubPass> subptr;

    std::unordered_map<llvm::Function*, subptr> passes;

    SubPass* getDefaultSubPass() const {
        return passes.at(nullptr).get();
    }

    subptr createSubPass() {
        return subptr(new SubPass(this));
    }

public:

    static char ID;

    PassModularizer() : llvm::ModulePass(ID) {
        passes[nullptr] = createSubPass();
    }

    virtual bool runOnModule(llvm::Module &M) {
        bool changed = false;

        for (auto& F : M) {
            // Do not run on declarations
            if (F.isDeclaration()) continue;

            subptr ptr(createSubPass());
            changed |= ptr->runOnFunction(F);
            passes[&F] = std::move(ptr);
        }

        return changed;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        getDefaultSubPass()->getAnalysisUsage(AU);
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

template<class SubPass>
struct RegisterModularizedPass {
    RegisterModularizedPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) {

        static llvm::RegisterPass< PassModularizer<SubPass> >
        MX(PassArg, Name, CFGOnly, isAnalysis);

    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PASSMODULIZER_HPP_ */
