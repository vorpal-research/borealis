/*
 * PassModulizer.hpp
 *
 *  Created on: Feb 8, 2013
 *      Author: belyaev
 */

#ifndef PASSMODULIZER_HPP_
#define PASSMODULIZER_HPP_

#include <unordered_map>
#include <utility>

#include <llvm/Pass.h>


namespace borealis {

template<class SubPass>
class PassModulizer : llvm::ModulePass {

    typedef std::unique_ptr<SubPass> subptr;

    std::unordered_map<llvm::Function*, subptr> passes;

    SubPass* getDefaultSubPass() const{
        return passes.at(nullptr).get();
    }

public:
    static char ID;

    PassModulizer(): llvm::ModulePass(ID){
        passes[nullptr] = subptr(new SubPass());
    };

    /// runOnModule - Virtual method overriden by subclasses to process the module
    /// being operated on.
    virtual bool runOnModule(llvm::Module &M) {
        bool changed = false;

        for(auto& F : M) {
            subptr ptr(new SubPass());
            changed |= ptr->runOnFunction(F);
            passes.insert(&F, std::move(ptr));
        }

        return changed;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        getDefaultSubPass()->getAnalysisUsage(AU);
    }

    SubPass& getResultsForFunction(llvm::Function* F) {
        return *passes[F];
    }

};

}// namespace borealis



#endif /* PASSMODULIZER_HPP_ */
