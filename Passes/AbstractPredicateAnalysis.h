/*
 * AbstractPredicateAnalysis.h
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#ifndef ABSTRACTPREDICATEANALYSIS_H_
#define ABSTRACTPREDICATEANALYSIS_H_

#include <llvm/BasicBlock.h>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>

#include <map>
#include <set>

#include "Predicate/Predicate.h"
#include "Util/util.h"

namespace borealis {

class AbstractPredicateAnalysis: public llvm::FunctionPass {

public:

    typedef std::map<const llvm::Instruction*, Predicate::Ptr> PredicateMap;
    typedef std::pair<const llvm::Instruction*, Predicate::Ptr> PredicateMapEntry;

    typedef std::pair<const llvm::TerminatorInst*, const llvm::BasicBlock*> TerminatorBranch;
    typedef std::map<TerminatorBranch, Predicate::Ptr> TerminatorPredicateMap;
    typedef std::pair<TerminatorBranch, Predicate::Ptr> TerminatorPredicateMapEntry;

    typedef std::pair<const llvm::BasicBlock*, const llvm::PHINode*> PhiBranch;
    typedef std::map<PhiBranch, Predicate::Ptr> PhiPredicateMap;
    typedef std::pair<PhiBranch, Predicate::Ptr> PhiPredicateMapEntry;

    typedef std::set<const void*> RegisteredPasses;

    AbstractPredicateAnalysis(char ID);
    virtual bool runOnFunction(llvm::Function& F) = 0;
    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const = 0;
    virtual ~AbstractPredicateAnalysis();

    PredicateMap& getPredicateMap() { return PM; }
    TerminatorPredicateMap& getTerminatorPredicateMap() { return TPM; }
    PhiPredicateMap& getPhiPredicateMap() { return PPM; }

    static RegisteredPasses getRegistered() {
        return registeredPasses;
    }

protected:

    virtual void init() {
        PM.clear();
        TPM.clear();
        PPM.clear();
    }

    PredicateMap PM;
    TerminatorPredicateMap TPM;
    PhiPredicateMap PPM;

    static RegisteredPasses registeredPasses;

};

} /* namespace borealis */

#endif /* ABSTRACTPREDICATEANALYSIS_H_ */
