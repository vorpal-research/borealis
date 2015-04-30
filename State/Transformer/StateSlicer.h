/*
 * StateSlicer.h
 *
 *  Created on: Mar 16, 2015
 *      Author: ice-phoenix
 */

#ifndef STATE_TRANSFORMER_STATESLICER_H_
#define STATE_TRANSFORMER_STATESLICER_H_

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/AliasSetTracker.h>

#include <unordered_set>

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class StateSlicer : public borealis::Transformer<StateSlicer> {

    using Base = borealis::Transformer<StateSlicer>;

public:

    StateSlicer(FactoryNest FN, PredicateState::Ptr query, llvm::AliasAnalysis* AA);

    PredicateState::Ptr transform(PredicateState::Ptr ps);
    using Base::transformBase;
    Predicate::Ptr transformBase(Predicate::Ptr pred);

private:

    PredicateState::Ptr query;

    llvm::AliasAnalysis* AA;
    std::unique_ptr<llvm::AliasSetTracker> AST;

    Term::Set sliceVars;
    Term::Set slicePtrs;

    void init();
    void addSliceTerm(Term::Ptr term);

    bool checkPath(Predicate::Ptr pred, const Term::Set& lhv, const Term::Set& rhv);
    bool checkVars(const Term::Set& lhv, const Term::Set& rhv);
    bool checkPtrs(const Term::Set& lhv, const Term::Set& rhv);

    bool aliases(Term::Ptr a, Term::Ptr b);

    llvm::Value* term2value(Term::Ptr t);

    uint64_t getLLVMAliasSize(llvm::Type* t);
};

} /* namespace borealis */

#endif /* STATE_TRANSFORMER_STATESLICER_H_ */
