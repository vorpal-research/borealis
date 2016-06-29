//
// Created by belyaev on 4/3/15.
//

#ifndef AURORA_SANDBOX_VARIABLECOLLECTOR_H
#define AURORA_SANDBOX_VARIABLECOLLECTOR_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class VariableCollector: public Transformer<VariableCollector> {

    std::unordered_set<Term::Ptr, TermHash, TermEquals> collection;

public:
    VariableCollector(const FactoryNest& FN) : Transformer(FN) { }

    Term::Ptr transformValueTerm(ValueTermPtr term) {
        collection.insert(term);
        return term;
    }

    Term::Ptr transformArgumentTerm(ArgumentTermPtr term) {
        collection.insert(term);
        return term;
    }

    Term::Ptr transformConstTerm(ConstTermPtr term) {
        collection.insert(term);
        return term;
    }

    Term::Ptr transformFreeVarTerm(FreeVarTermPtr term) {
        collection.insert(term);
        return term;
    }

    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr term) {
        collection.insert(term);
        return term;
    }

    Term::Ptr transformReturnPtrTerm(ReturnPtrTermPtr term) {
        collection.insert(term);
        return term;
    }

    const std::unordered_set<Term::Ptr, TermHash, TermEquals>& getCollectedTerms() const {
        return collection;
    }

    std::unordered_set<Term::Ptr, TermHash, TermEquals>& getCollectedTerms() {
        return collection;
    }
};

template<class ...Transformables>
std::unordered_set<Term::Ptr, TermHash, TermEquals> collectVariables(FactoryNest FN, Transformables&... someLogic) {
    VariableCollector vc(FN);
    util::use(vc.transform(someLogic)...);
    return std::move(vc.getCollectedTerms());
}

} /* namespace borealis */


#endif //AURORA_SANDBOX_VARIABLECOLLECTOR_H
