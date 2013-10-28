/*
 * Simplifier.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef SIMPLIFIER_H_
#define SIMPLIFIER_H_

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class Simplifier : public borealis::Transformer<Simplifier> {

    typedef borealis::Transformer<Simplifier> Base;

    unsigned int level;
    std::vector<Term::Ptr> axs;

public:

    Simplifier(FactoryNest FN) : Base(FN), level(0U) {}

    using Base::transform;

    // XXX: Should not be shadowed in your everyday transformers
    //      Don't try this at home!
    Term::Ptr transform(Term::Ptr term) {
        using borealis::util::head;
        using borealis::util::tail;

        ++level;
        auto res = transformBase(term);
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

    Term::Ptr transformAxiomTerm(AxiomTermPtr t) {
        axs.push_back(t->getRhv());
        return t->getLhv();
    }

};

} /* namespace borealis */

#endif /* SIMPLIFIER_H_ */
