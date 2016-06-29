//
// Created by belyaev on 4/3/15.
//

#ifndef AURORA_SANDBOX_POINTERCOLLECTOR_H
#define AURORA_SANDBOX_POINTERCOLLECTOR_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class PointerCollector: public Transformer<PointerCollector> {

    std::unordered_set<Term::Ptr, TermHash, TermEquals> collection;

public:
    PointerCollector(const FactoryNest& FN) : Transformer(FN) { }

    Term::Ptr transformLoadTerm(LoadTermPtr term) {
        auto ptr = term->getRhv();
        collection.insert(ptr);
        return term;
    }

    Predicate::Ptr transformStorePredicate(StorePredicatePtr pred) {
        auto&& ptr = pred->getLhv();
        collection.insert(ptr);
        return pred;
    }

    const std::unordered_set<Term::Ptr, TermHash, TermEquals>& getCollectedTerms() const {
        return collection;
    }

    std::unordered_set<Term::Ptr, TermHash, TermEquals>& getCollectedTerms() {
        return collection;
    }
};

template<class ...Transformables>
std::unordered_set<Term::Ptr, TermHash, TermEquals> collectPointers(FactoryNest FN, Transformables&... someLogic) {
    PointerCollector vc(FN);
    util::use(vc.transform(someLogic)...);
    return std::move(vc.getCollectedTerms());
}

} /* namespace borealis */


#endif //AURORA_SANDBOX_POINTERCOLLECTOR_H
