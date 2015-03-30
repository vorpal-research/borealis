/*
 * StateSlicer.cpp
 *
 *  Created on: Mar 16, 2015
 *      Author: ice-phoenix
 */

#include "State/Transformer/StateSlicer.h"
#include "State/Transformer/TermCollector.h"

#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

StateSlicer::StateSlicer(FactoryNest FN, PredicateState::Ptr query, llvm::AliasAnalysis* AA) :
        Base(FN), query(query), AA(AA) { init(); }

struct isInterestingTerm {
    bool operator()(Term::Ptr t) const {
        return llvm::isa<ArgumentTerm>(t) ||
                llvm::isa<ReturnValueTerm>(t) ||
                llvm::isa<ValueTerm>(t);
    }
};

void StateSlicer::init() {
    auto&& tc = TermCollector(FN);
    tc.transform(query);

    util::viewContainer(tc.getTerms())
    .filter(isInterestingTerm())
    .foreach(APPLY(this->addSliceTerm));
}

void StateSlicer::addSliceTerm(Term::Ptr term) {
    if (llvm::isa<type::Pointer>(term->getType())) {
        slicePtrs.insert(term);
    } else {
        sliceVars.insert(term);
    }
}

Predicate::Ptr StateSlicer::transformPredicate(Predicate::Ptr pred) {
    auto&& lhvTerms = Term::Set{};
    for (auto&& lhv : util::viewContainer(pred->getOperands()).take(1)) {
        auto&& nested = Term::getFullTermSet(lhv);
        util::viewContainer(nested)
                .filter(isInterestingTerm())
                .foreach(APPLY(lhvTerms.insert));
    }
    auto&& rhvTerms = Term::Set{};
    for (auto&& rhv : util::viewContainer(pred->getOperands()).drop(1)) {
        auto&& nested = Term::getFullTermSet(rhv);
        util::viewContainer(nested)
                .filter(isInterestingTerm())
                .foreach(APPLY(rhvTerms.insert));
    }
    // FIXME: akhin Process pointers w.r.t. aliasing
    if (
        util::viewContainer(lhvTerms)
        .any_of([&](auto&& t) { return util::contains(sliceVars, t); })
    ) {
        util::viewContainer(rhvTerms)
        .foreach(APPLY(this->addSliceTerm));
    }

    return pred;
}

#include "Util/unmacros.h"

} /* namespace borealis */
