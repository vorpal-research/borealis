/*
 * Simplifier.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

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

    if (op == llvm::UnaryArithType::NOT) {
        if (auto* cmp = llvm::dyn_cast<CmpTerm>(rhv)) {
            return FN.Term->getCmpTerm(
                makeNot(cmp->getOpcode()),
                cmp->getLhv(),
                cmp->getRhv()
            );
        }
    }

    return t;
}

} /* namespace borealis */


