/*
 * Simplifier.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#include <numeric>
#include <algorithm>

#include "State/Transformer/Simplifier.h"

namespace borealis {

Term::Ptr Simplifier::transformBase(Term::Ptr term) {
    using borealis::util::head;
    using borealis::util::tail;

    ++level;
    auto res = Base::transformBase(term);
    --level;

    if (level != 0) return res;

    if (axs.empty()) return res;

    auto acc = head(axs);
    auto rest = tail(axs);
    auto axsTerm = std::accumulate(rest.begin(), rest.end(), acc,
                                   [this](Term::Ptr acc, Term::Ptr next) {
                                       return FN.Term->getBinaryTerm(llvm::ArithType::LAND, acc, next);
                                   }
    );

    return FN.Term->getAxiomTerm(res, axsTerm);
}

Term::Ptr Simplifier::transformAxiomTerm(AxiomTermPtr t) {
    axs.push_back(t->getRhv());
    return t->getLhv();
}

Term::Ptr Simplifier::transformUnaryTerm(UnaryTermPtr t) {
    auto op = t->getOpcode();
    auto rhv = t->getRhv();

    auto tru = FN.Term->getTrueTerm();
    auto fls = FN.Term->getFalseTerm();

    if (op == llvm::UnaryArithType::NOT) {
        if (auto* cmp = llvm::dyn_cast<CmpTerm>(rhv)) {
            return FN.Term->getCmpTerm(
                makeNot(cmp->getOpcode()),
                cmp->getLhv(),
                cmp->getRhv()
            );
        }

        if(*rhv == *tru) return fls;
        if(*rhv == *fls) return tru;
    }

    return t;
}

Term::Ptr Simplifier::transformBinaryTerm(BinaryTermPtr t) {
    auto op = t->getOpcode();
    auto lhv = t->getLhv();
    auto rhv = t->getRhv();

    auto tru = FN.Term->getTrueTerm();
    auto fls = FN.Term->getFalseTerm();

    if(op == llvm::ArithType::LAND && (*lhv == *tru)) return rhv;
    if(op == llvm::ArithType::LAND && (*rhv == *tru)) return lhv;
    if(op == llvm::ArithType::LAND && (*lhv == *fls)) return fls;
    if(op == llvm::ArithType::LAND && (*rhv == *fls)) return fls;

    if(op == llvm::ArithType::LOR && (*lhv == *fls)) return rhv;
    if(op == llvm::ArithType::LOR && (*rhv == *fls)) return lhv;
    if(op == llvm::ArithType::LOR && (*lhv == *tru)) return tru;
    if(op == llvm::ArithType::LOR && (*rhv == *tru)) return tru;

    return t;
}

Term::Ptr Simplifier::transformCmpTerm(CmpTermPtr t) {
    auto op = t->getOpcode();
    auto lhv = t->getLhv();
    auto rhv = t->getRhv();

    auto tru = FN.Term->getTrueTerm();
    auto fls = FN.Term->getFalseTerm();

    switch(op) {
    case llvm::ConditionType::NEQ:
    case llvm::ConditionType::GT:
    case llvm::ConditionType::UGT:
    case llvm::ConditionType::LT:
    case llvm::ConditionType::ULT:
        if(*lhv == *rhv) return fls;
    case llvm::ConditionType::FALSE:
        return fls;
    case llvm::ConditionType::TRUE:
        return tru;
    default:
        return t;
    }
}

} /* namespace borealis */


