#include <Term/TermUtils.hpp>
#include "State/Transformer/PoorMem2Reg.h"

#include <functional-hell/matchers_fancy_syntax.h>

namespace borealis {

PredicateState::Ptr PoorMem2Reg::transformChoice(PredicateStateChoicePtr choice) {
    choice->fmap([this](auto&& ch) -> PredicateState::Ptr {
        PoorMem2Reg child = *this;
        auto res = child.transform(ch);
        for(auto ms : child.invalidatedMS) {
            mapping.erase(ms);
        }
        return res;
    });
    return choice;
}

Predicate::Ptr PoorMem2Reg::transformStore(Transformer::StorePredicatePtr store) {
    auto ms = TypeUtils::getPointerMemorySpace(store->getLhv()->getType());
    mapping[ms].memory.clear();
    mapping[ms].memory[store->getLhv()] = store->getRhv();
    invalidatedMS.insert(ms);
    return store;
}

Predicate::Ptr PoorMem2Reg::transformWriteBound(Transformer::WriteBoundPredicatePtr wb) {
    auto ms = TypeUtils::getPointerMemorySpace(wb->getLhv()->getType());
    mapping[ms].bounds.clear();
    mapping[ms].bounds[wb->getLhv()] = wb->getRhv();
    invalidatedMS.insert(ms);
    return wb;
}

Predicate::Ptr PoorMem2Reg::transformWriteProperty(Transformer::WritePropertyPredicatePtr wp) {
    auto prop = TermUtils::getStringValue(wp->getPropertyName()).getUnsafe();
    auto ms = TypeUtils::getPointerMemorySpace(wp->getLhv()->getType());
    mapping[ms].properties[prop].clear();
    mapping[ms].properties[prop][wp->getLhv()] = wp->getRhv();
    invalidatedMS.insert(ms);
    return wp;
}

Term::Ptr PoorMem2Reg::transformLoadTerm(LoadTermPtr term) {
    auto ms = TypeUtils::getPointerMemorySpace(term->getRhv()->getType());
    auto&& mem = mapping[ms].memory;
    auto it = mem.find(term->getRhv());
    if(it != mem.end()) return it->second;
    else return term;
}

Term::Ptr PoorMem2Reg::transformBoundTerm(BoundTermPtr term) {
    auto ms = TypeUtils::getPointerMemorySpace(term->getRhv()->getType());
    auto&& mem = mapping[ms].bounds;
    auto it = mem.find(term->getRhv());
    if(it != mem.end()) return it->second;
    else return term;
}

Term::Ptr PoorMem2Reg::transformReadPropertyTerm(ReadPropertyTermPtr term) {
    auto prop = TermUtils::getStringValue(term->getPropertyName()).getUnsafe();
    auto ms = TypeUtils::getPointerMemorySpace(term->getRhv()->getType());
    auto&& mem = mapping[ms].properties[prop];
    auto it = mem.find(term->getRhv());
    if(it != mem.end()) return it->second;
    else return term;
}

template<class Lhv, class Rhv>
static auto $EqualityTerm(Lhv&& lhv, Rhv&& rhv) -> decltype(auto) {
    return $CmpTerm(llvm::ConditionType::EQ, lhv, rhv);
};

Predicate::Ptr PoorMem2Reg::transformAlloca(Transformer::AllocaPredicatePtr alloc) {
    auto ptr = alloc->getLhv();
    auto ms = TypeUtils::getPointerMemorySpace(ptr->getType());
    auto bound = alloc->getOrigNumElems();
    mapping[ms].bounds[ptr] = bound;
    return alloc;
}

Predicate::Ptr PoorMem2Reg::transformEquality(Transformer::EqualityPredicatePtr eq_) {

    using namespace functional_hell::matchers::placeholders;

    auto eq = Base::transformEquality(eq_);

    auto $True = $OpaqueBoolConstantTerm(true);

    SWITCH(eq) {
        NAMED_CASE(m, $EqualityPredicate($EqualityTerm($LoadTerm(_1), _2), $True)
                    | $EqualityPredicate($EqualityTerm(_2, $LoadTerm(_1)), $True)) {
            Term::Ptr ptr = m->_1;
            Term::Ptr val = m->_2;

            auto ms = TypeUtils::getPointerMemorySpace(ptr->getType());

            mapping[ms].memory[ptr] = val;
            invalidatedMS.insert(ms);
        }

        NAMED_CASE(m, $EqualityPredicate($EqualityTerm($BoundTerm(_1), _2), $True)
                    | $EqualityPredicate($EqualityTerm(_2, $BoundTerm(_1)), $True)) {
            Term::Ptr ptr = m->_1;
            Term::Ptr val = m->_2;

            auto ms = TypeUtils::getPointerMemorySpace(ptr->getType());
            mapping[ms].bounds[ptr] = val;
            invalidatedMS.insert(ms);
        }

        NAMED_CASE(m, $EqualityPredicate($EqualityTerm($ReadPropertyTerm(_3, _1), _2), $True)
                    | $EqualityPredicate($EqualityTerm(_2, $ReadPropertyTerm(_3, _1)), $True)) {
            auto&& property = m->_3;
            Term::Ptr ptr = m->_1;
            Term::Ptr val = m->_2;

            auto ms = TypeUtils::getPointerMemorySpace(ptr->getType());

            mapping[ms].properties[property][ptr] = val;
            invalidatedMS.insert(ms);
        }
    }

    return eq;
}

} /* namespace borealis */


