//
// Created by abdullin on 11/1/17.
//

#include "Interpreter/Domain/DomainFactory.h"
#include "Interpreter/Checker/OutOfBoundsVisitor.h"
#include "OutOfBoundsChecker.h"

#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

OutOfBoundsChecker::OutOfBoundsChecker(FactoryNest FN, DefectManager* DM, DomainFactory* DF, Interpreter* interpreter)
        : Transformer(FN), ObjectLevelLogging("ps-interpreter"), DM_(DM), DF_(DF), interpreter_(interpreter) {}

PredicateState::Ptr OutOfBoundsChecker::transformBasic(BasicPredicateStatePtr basic) {
    currentBasic_ = basic;
    return Base::transformBasic(basic);
}

Predicate::Ptr OutOfBoundsChecker::transformPredicate(Predicate::Ptr pred) {
    currentLocus_ = &pred->getLocation();
    return Base::transformPredicate(pred);
}

Term::Ptr OutOfBoundsChecker::transformGepTerm(GepTermPtr term) {
    auto state = interpreter_->getStateMap().at(currentBasic_);
    auto di = DefectInfo{DefectTypes.at(DefectType::BUF_01).type, *currentLocus_};

    auto ptr = state->find(term->getBase());
    ASSERT(ptr, "gep pointer of " + term->getName());

    std::vector<Domain::Ptr> shifts;
    for (auto&& it : term->getShifts()) {
        auto shift = state->find(it);
        ASSERT(shift, "Unknown shift: " + it->getName());
        auto&& integer = llvm::dyn_cast<IntegerIntervalDomain>(shift.get());
        ASSERT(integer, "Non-integer domain in gep shift");

        if (integer->getWidth() < DF_->defaultSize) {
            shifts.emplace_back(shift->zext(FN.Type->getInteger(DF_->defaultSize)));
        } else if (integer->getWidth() > DF_->defaultSize) {
            shifts.emplace_back(shift->trunc(FN.Type->getInteger(DF_->defaultSize)));
        } else {
            shifts.emplace_back(shift);
        }
    }
    auto bug = OutOfBoundsVisitor().visit(ptr, shifts);
    defects_[di] |= bug;
    return term;
}

void OutOfBoundsChecker::apply() {
    for (auto&& it : defects_) {
        if (not it.second) {
            errs() << "Skipped defect: " << it.first << endl;
        }
    }
    util::viewContainer(defects_)
            .filter(LAM(a, not a.second))
            .foreach([&](auto&& it) -> void { DM_->addNoAbsIntDefect(it.first); });
}

}   // namespace ps
}   // namespace absint
}   // namespace borealis

#include "Util/unmacros.h"