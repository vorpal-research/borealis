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

static Predicate::Ptr inverse(FactoryNest& FN, const Predicate::Ptr& predicate) {
    using namespace functional_hell::matchers;
    using namespace functional_hell::matchers::placeholders;
    if(auto m = $EqualityPredicate(_1, $OpaqueBoolConstantTerm(true)) >> predicate) {
        return FN.Predicate->getEqualityPredicate(m->_1, FN.Term->getFalseTerm(), predicate->getLocation(), predicate->getType());
    }
    if(auto m = $EqualityPredicate(_1, $OpaqueBoolConstantTerm(false)) >> predicate) {
        return FN.Predicate->getEqualityPredicate(m->_1, FN.Term->getTrueTerm(), predicate->getLocation(), predicate->getType());
    }
    return nullptr;
}

static std::unordered_set<Predicate::Ptr> inverseSwitch(FactoryNest& FN, const Predicate::Ptr& predicate) {
    if(auto dscp = llvm::dyn_cast<DefaultSwitchCasePredicate>(predicate)) {
        auto cond = dscp->getCond();
        return dscp->getCases()
              .map(LAM(value, FN.Predicate->getEqualityPredicate(cond, value, predicate->getLocation(), predicate->getType())))
              .toHashSet();
    }
    return {};
}

StateSlicer::StateSlicer(FactoryNest FN, PredicateState::Ptr query, llvm::AliasAnalysis* AA) :
    Base(FN), query(query), sliceVars{}, slicePtrs{}, AA{}, CFDT{FN}{ init(AA); }

StateSlicer::StateSlicer(FactoryNest FN, PredicateState::Ptr query) :
    Base(FN), query(query), sliceVars{}, slicePtrs{}, AA{}, CFDT{FN}{ init(nullptr); }


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
        return TermUtils::isNamedTerm(t);
    }
} isInterestingTerm;

void StateSlicer::init(llvm::AliasAnalysis* llvmAA) {
    if(llvmAA) {
        AA = util::make_unique<AliasAnalysisAdapter>(llvmAA, FN);
    } else {
        AA = util::make_unique<LocalStensgaardAA>(FN);
    }

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
    if(AA) AA->prepare(ps);
    CFDT.reset();
    CFDT.transform(ps);
    currentPathDeps = CFDT.getFinalPaths();

    if(nullptr == AA) return ps;

    auto reversed = ps->reverse();
    return Base::transform(reversed)
           ->filter([](auto&& p) { return !!p; })
           ->reverse()
           ->simplify();
}


PredicateState::Ptr StateSlicer::transformBase(PredicateState::Ptr ps) {
    return Base::transformBase(ps);
}

PredicateState::Ptr StateSlicer::transformChoice(PredicateStateChoicePtr ps) {
    // TODO

    auto psi = ps->simplify();

    if(auto choice = llvm::dyn_cast<PredicateStateChoice>(psi)) {
        auto savedPathDeps = currentPathDeps;
        auto result = choice->fmap([this, savedPathDeps](auto&& st){
            currentPathDeps = savedPathDeps;
            return Base::transformBase(st);
        });
        currentPathDeps = savedPathDeps;
        return result;
    } else return Base::transformBase(psi);
}

void StateSlicer::addControlFlowDeps(Predicate::Ptr res) {
    currentPathDeps = CFDT.getDominatingPaths(res);
}

Predicate::Ptr StateSlicer::transformBase(Predicate::Ptr pred) {

    if(auto&& globals = llvm::dyn_cast<GlobalsPredicate>(pred)) {
        // by the global definition, everything relevant must be in slice
        // we count only pointers because all globals are pointers
        auto data = globals->getGlobals().filter(LAM(t, slicePtrs.count(t))).toVector();
        if(data.empty()) return nullptr;
        return FN.Predicate->getGlobalsPredicate(data, globals->getLocation());
    }

    if (nullptr == AA) return pred;
    if (PredicateType::STATE != pred->getType() && PredicateType::INVARIANT != pred->getType()){
        auto anti = inverse(FN, pred);
        if(currentPathDeps.count(pred) && not currentPathDeps.count(anti)) {
            for (auto&& e : util::viewContainer(pred->getOperands())) {
                auto&& nested = Term::getFullTermSet(e);
                util::viewContainer(nested)
                    .filter(isInterestingTerm)
                    .foreach(APPLY(this->addSliceTerm));
            }
            addControlFlowDeps(pred);
            return pred;
        }
        return nullptr;
    }

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

    if (checkVars(lhvTerms, rhvTerms)) {
        res = pred;
    } else if (checkPtrs(pred, lhvTerms, rhvTerms)) {
        res = pred;
    }

    if(res) {
        addControlFlowDeps(res);
    }

    return res;
}

bool StateSlicer::checkPath(Predicate::Ptr pred, const Term::Set& lhv, const Term::Set& rhv) {
    if (PredicateType::PATH == pred->getType() ||
        PredicateType::ASSUME == pred->getType() ||
        PredicateType::REQUIRES == pred->getType()) {

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

bool StateSlicer::checkPtrs(Predicate::Ptr pred, const Term::Set& lhv, const Term::Set& rhv) {
    if(lhv.empty()) return false;

    auto added = false;
    if (
        util::viewContainer(lhv)
             .filter(isPointerTerm)
             .any_of(LAM(t, slicePtrs.count(t)))
    ) {
        util::viewContainer(rhv)
            .foreach(APPLY(this->addSliceTerm));
        added = true;
    }
    if(added) return added;

    if(llvm::is_one_of<
                StorePredicate,
                WriteBoundPredicate,
                WritePropertyPredicate,
                AllocaPredicate,
                MallocPredicate,
                SeqDataPredicate,
                SeqDataZeroPredicate
            >(pred)) {

        if (util::viewContainer(lhv)
                .filter(isPointerTerm)
                .any_of([&](auto&& a) {
                    return util::viewContainer(slicePtrs)
                        .any_of([&](auto&& b) {
                            return AA->mayAlias(a, b);
                        });
                })
            ) {
            util::viewContainer(lhv)
                .foreach(APPLY(this->addSliceTerm));
            util::viewContainer(rhv)
                .foreach(APPLY(this->addSliceTerm));
            added = true;
        }
    }

    return added;
}

#include "Util/unmacros.h"

} /* namespace borealis */
