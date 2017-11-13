//
// Created by abdullin on 11/7/17.
//

#include "NullDereferenceChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

NullDereferenceChecker::NullDereferenceChecker(FactoryNest FN, DefectManager* DM, Interpreter* interpreter)
        : Transformer(FN), ObjectLevelLogging("ps-interpreter"), DM_(DM), interpreter_(interpreter) {}

PredicateState::Ptr NullDereferenceChecker::transformBasic(BasicPredicateStatePtr basic) {
    currentBasic_ = basic;
    return Base::transformBasic(basic);
}

Predicate::Ptr NullDereferenceChecker::transformPredicate(Predicate::Ptr pred) {
    currentLocus_ = &pred->getLocation();
    return Base::transformPredicate(pred);
}

Predicate::Ptr NullDereferenceChecker::transformStorePredicate(Transformer::StorePredicatePtr pred) {
    checkPtr(pred->getLhv());
    return pred;
}

Term::Ptr NullDereferenceChecker::transformGepTerm(GepTermPtr term) {
    checkPtr(term->getBase());
    return term;
}

Term::Ptr NullDereferenceChecker::transformLoadTerm(Transformer::LoadTermPtr term) {
    checkPtr(term->getRhv());
    return term;
}

void NullDereferenceChecker::apply() {
    util::viewContainer(defects_)
            .filter(LAM(a, not a.second))
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

void NullDereferenceChecker::checkPtr(Term::Ptr ptr) {
    auto state = interpreter_->getStateMap().at(currentBasic_);
    auto di = DefectInfo{DefectTypes.at(DefectType::NDF_01).type, *currentLocus_};

    auto ptr_domain = state->find(ptr);
    ASSERT(ptr, "gep pointer of " + ptr->getName());

    defects_[di] |= not ptr_domain->isValue() || ptr_domain->isNullptr();
}

}   // namespace ps
}   // namespace absint
}   // namespace borealis

#include "Util/unmacros.h"
