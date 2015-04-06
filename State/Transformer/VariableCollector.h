//
// Created by belyaev on 4/3/15.
//

#ifndef AURORA_SANDBOX_VARIABLECOLLECTOR_H
#define AURORA_SANDBOX_VARIABLECOLLECTOR_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

struct TermNameHash {
    size_t operator()(Term::Ptr trm) const noexcept {
        return util::hash::simple_hash_value(trm->getName());
    }
};

struct TermNameEquals {
    bool operator()(Term::Ptr lhv, Term::Ptr rhv) const noexcept {
        return lhv->getName() == rhv->getName();
    }
};

class VariableCollector: public Transformer<VariableCollector> {

    std::unordered_set<Term::Ptr, TermNameHash, TermNameEquals> collection;

public:
    VariableCollector(const FactoryNest& FN) : Transformer(FN) { }

    Term::Ptr transformValueTerm(ValueTermPtr term) {
        collection.insert(term);
        return term;
    }

    const std::unordered_set<Term::Ptr, TermNameHash, TermNameEquals>& getCollectedTerms() const {
        return collection;
    }

    std::unordered_set<Term::Ptr, TermNameHash, TermNameEquals>& getCollectedTerms() {
        return collection;
    }
};

template<class ...Transformables>
std::unordered_set<Term::Ptr, TermNameHash, TermNameEquals> collectVariables(FactoryNest FN, Transformables&... someLogic) {
    VariableCollector vc(FN);
    util::use(vc.transform(someLogic)...);
    return std::move(vc.getCollectedTerms());
}

} /* namespace borealis */


#endif //AURORA_SANDBOX_VARIABLECOLLECTOR_H
