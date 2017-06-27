#ifndef NORMALIZER_H
#define NORMALIZER_H


#include "State/Transformer/Transformer.hpp"

namespace borealis {

class Normalizer: public Transformer<Normalizer> {

    std::vector<Term::Ptr> accumulator;
    Predicate::Ptr tombstone;
    bool handleAnd(Term::Ptr term);

public:
    Normalizer(FactoryNest FN): Transformer(FN) {
        tombstone = FN.Predicate->getMarkPredicate(FN.Term->getNullPtrTerm());
    }

    Predicate::Ptr transformEquality(EqualityPredicatePtr eq);
    PredicateState::Ptr transformBasic(BasicPredicateStatePtr basic);
};

} /* namespace borealis */

#endif // NORMALIZER_H
