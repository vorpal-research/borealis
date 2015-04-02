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
    Base(FN), query(query), AA(AA), AST(*AA) { init(); }

static struct {
    using argument_type = Term::Ptr;

    bool operator()(Term::Ptr t) const {
        return llvm::isa<type::Pointer>(t->getType());
    }
} isPointerTerm;

static auto isNotPointerTerm = std::not1(isPointerTerm);

static struct {
    using argument_type = Term::Ptr;

    bool operator()(Term::Ptr t) const {
        return llvm::isa<ArgumentTerm>(t) ||
               llvm::isa<ValueTerm>(t);
    }
} isInterestingTerm;

void StateSlicer::init() {
    auto&& tc = TermCollector(FN);
    tc.transform(query);

    util::viewContainer(tc.getTerms())
        .filter(isInterestingTerm)
        .foreach(APPLY(this->addSliceTerm));
}

void StateSlicer::addSliceTerm(Term::Ptr term) {
    if (isPointerTerm(term)) {
        slicePtrs.insert(term);
    } else {
        sliceVars.insert(term);
    }
}

PredicateState::Ptr StateSlicer::transform(PredicateState::Ptr ps) {
    return Base::transform(ps->reverse())->filter([](auto&& p) { return !!p; })->reverse();
}

Predicate::Ptr StateSlicer::transformPredicate(Predicate::Ptr pred) {
    auto&& lhvTerms = Term::Set{};
    for (auto&& lhv : util::viewContainer(pred->getOperands()).take(1)) {
        auto&& nested = Term::getFullTermSet(lhv);
        util::viewContainer(nested)
            .filter(isInterestingTerm)
            .foreach(APPLY(lhvTerms.insert));
    }
    auto&& rhvTerms = Term::Set{};
    for (auto&& rhv : util::viewContainer(pred->getOperands()).drop(1)) {
        auto&& nested = Term::getFullTermSet(rhv);
        util::viewContainer(nested)
            .filter(isInterestingTerm)
            .foreach(APPLY(rhvTerms.insert));
    }

    Predicate::Ptr res = nullptr;

    if (checkPath(pred, lhvTerms, rhvTerms)) {
        res = pred;
    } else if (checkVars(lhvTerms, rhvTerms)) {
        res = pred;
    } else if (checkPtrs(lhvTerms, rhvTerms)) {
        res = pred;
    }

    return res;
}

bool StateSlicer::checkPath(Predicate::Ptr pred, const Term::Set& lhv, const Term::Set& rhv) {
    if (PredicateType::PATH == pred->getType() ||
        PredicateType::ASSUME == pred->getType()) {
        (util::viewContainer(lhv) >> util::viewContainer(rhv))
            .foreach(APPLY(this->addSliceTerm));
        return true;
    }
    return false;
}

bool StateSlicer::checkVars(const Term::Set& lhv, const Term::Set& rhv) {
    if (
        util::viewContainer(lhv)
            .filter(isNotPointerTerm)
            .any_of([&](auto&& t) { return util::contains(sliceVars, t); })
        ) {
        util::viewContainer(rhv)
            .foreach(APPLY(this->addSliceTerm));
        return true;
    }
    return false;
}

bool StateSlicer::checkPtrs(const Term::Set& lhv, const Term::Set& rhv) {
    if (
        util::viewContainer(lhv)
            .filter(isPointerTerm)
            .any_of([&](auto&& a) {
                return util::viewContainer(slicePtrs)
                    .any_of([&](auto&& b) {
                        return aliases(a, b);
                    });
            })
        ) {
        util::viewContainer(rhv)
            .foreach(APPLY(this->addSliceTerm));
        return true;
    }
    return false;
}

bool StateSlicer::aliases(Term::Ptr a, Term::Ptr b) {
    auto&& p = term2value(a);
    auto&& q = term2value(b);

#define AS_POINTER(V) V, AA->getTypeStoreSize(V->getType()->getPointerElementType()), nullptr

    if (p and q) {
        AST.add(AS_POINTER(p));
        AST.add(AS_POINTER(q));
        return AST.getAliasSetForPointer(AS_POINTER(p)).aliasesPointer(AS_POINTER(q), *AA);
    } else {
        return true;
    }

#undef AS_POINTER
}

llvm::Value* StateSlicer::term2value(Term::Ptr t) {
    if (auto* vt = llvm::dyn_cast<ValueTerm>(t)) {
        if (vt->isGlobal()) {
            return const_cast<llvm::Value*>(FN.Slot->getGlobalValue(vt->getVName()));
        } else {
            return const_cast<llvm::Value*>(FN.Slot->getLocalValue(vt->getVName()));
        }
    } else if (auto* at = llvm::dyn_cast<ArgumentTerm>(t)) {
        return const_cast<llvm::Value*>(FN.Slot->getLocalValue(at->getName()));
    } else {
        // FIXME: akhin Logging
        return nullptr;
    }
}

#include "Util/unmacros.h"

} /* namespace borealis */
