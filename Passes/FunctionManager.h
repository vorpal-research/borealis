/*
 * FunctionManager.h
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#ifndef FUNCTIONMANAGER_H_
#define FUNCTIONMANAGER_H_

#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include <unordered_map>

#include "Predicate/PredicateFactory.h"
#include "State/PredicateState.h"
#include "Term/TermFactory.h"

namespace borealis {

class FunctionManager: public llvm::ModulePass {

public:

    static char ID;

    typedef std::unordered_map<llvm::Function*, PredicateState> Data;
    typedef Data::value_type DataEntry;

    FunctionManager();
    virtual bool runOnModule(llvm::Module&);
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;
    virtual ~FunctionManager() {};

    void put(llvm::Function* F, const PredicateState& state);
    void update(llvm::Function* F, const PredicateState& state);
    const PredicateState& get(llvm::Function* F);

    const PredicateState& get(llvm::CallInst& CI, PredicateFactory* PF, TermFactory* TF);

private:

    Data data;

};

} /* namespace borealis */

#endif /* FUNCTIONMANAGER_H_ */
