/*
 * AbstractPredicateAnalysis.h
 *
 *  Created on: Aug 31, 2012
 *      Author: ice-phoenix
 */

#ifndef ABSTRACTPREDICATEANALYSIS_H_
#define ABSTRACTPREDICATEANALYSIS_H_

#include <llvm/Pass.h>

#include <map>

#include "Predicate/Predicate.h"

namespace borealis {

class AbstractPredicateAnalysis {

public:

    typedef std::map<const llvm::Instruction*, Predicate::Ptr> PredicateMap;
    typedef PredicateMap::value_type PredicateMapEntry;

    typedef std::pair<const llvm::TerminatorInst*, const llvm::BasicBlock*> TerminatorBranch;
    typedef std::map<TerminatorBranch, Predicate::Ptr> TerminatorPredicateMap;
    typedef TerminatorPredicateMap::value_type TerminatorPredicateMapEntry;

    typedef std::pair<const llvm::BasicBlock*, const llvm::PHINode*> PhiBranch;
    typedef std::map<PhiBranch, Predicate::Ptr> PhiPredicateMap;
    typedef PhiPredicateMap::value_type PhiPredicateMapEntry;

    AbstractPredicateAnalysis();
    virtual ~AbstractPredicateAnalysis();

    PredicateMap& getPredicateMap() { return PM; }
    TerminatorPredicateMap& getTerminatorPredicateMap() { return TPM; }
    PhiPredicateMap& getPhiPredicateMap() { return PPM; }

protected:

    virtual void init() {
        PM.clear();
        TPM.clear();
        PPM.clear();
    }

    PredicateMap PM;
    TerminatorPredicateMap TPM;
    PhiPredicateMap PPM;

};

} /* namespace borealis */

#endif /* ABSTRACTPREDICATEANALYSIS_H_ */
