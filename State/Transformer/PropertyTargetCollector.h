//
// Created by belyaev on 4/3/15.
//

#ifndef PROPERTY_TARGET_COLLECTOR_H
#define PROPERTY_TARGET_COLLECTOR_H

#include "State/Transformer/Transformer.hpp"

namespace borealis {

class PropertyTargetCollector: public Transformer<PropertyTargetCollector> {

    using vpair = std::pair<std::string, Term::Ptr>;

    struct colHash{
        size_t operator()(const vpair& v) const noexcept{
            return util::hash::simple_hash_value(v.first, TermHash{}(v.second));
        }
    };

    struct colEq{
        bool operator()(const vpair& lhv,
                        const vpair& rhv) const {
            return lhv.first == rhv.first && TermEquals{}(lhv.second, rhv.second);
        }
    };

    std::unordered_set<vpair, colHash, colEq> collection;

public:
    using result_set = std::unordered_set<vpair, colHash, colEq>;

    PropertyTargetCollector(const FactoryNest& FN) : Transformer(FN) { }

    Term::Ptr transformReadPropertyTerm(ReadPropertyTermPtr term) {
        auto ptr = term->getRhv();
        auto prop = term->getPropertyName()->getName();
        collection.emplace(prop, ptr);
        return term;
    }

    Predicate::Ptr transformWritePropertyPredicate(WritePropertyPredicatePtr pred) {
        auto&& ptr = pred->getLhv();
        auto prop = pred->getPropertyName()->getName();
        collection.emplace(prop, ptr);
        return pred;
    }

    const result_set& getCollectedTerms() const {
        return collection;
    }

    result_set& getCollectedTerms() {
        return collection;
    }
};

template<class ...Transformables>
PropertyTargetCollector::result_set collectPropertyTargets(FactoryNest FN, Transformables&... someLogic) {
    PropertyTargetCollector vc(FN);
    util::use(vc.transform(someLogic)...);
    return std::move(vc.getCollectedTerms());
}

} /* namespace borealis */


#endif //PROPERTY_TARGET_COLLECTOR_H
