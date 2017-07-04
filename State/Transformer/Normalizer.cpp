#include "State/Transformer/Normalizer.h"

#include <functional-hell/matchers_fancy_syntax.h>

namespace borealis {

Predicate::Ptr Normalizer::transformEquality(Transformer::EqualityPredicatePtr eq) {
    auto $TrueTerm = $OpaqueBoolConstantTerm(true);
    using namespace functional_hell::matchers::placeholders;

    accumulator.clear();

    SWITCH(eq) {
        NAMED_CASE(m, $EqualityPredicate(_1, $TrueTerm)) {
            if(handleAnd(m->_1)) return tombstone;
        }
    }

    return eq;
}

PredicateState::Ptr Normalizer::transformBasic(Transformer::BasicPredicateStatePtr basic) {
    std::vector<Predicate::Ptr> ret;
    for(auto&& pred : basic->getData()) {
        auto r = transform(pred);
        if(PredicateShallowEquals{}(r, tombstone)) {
            for(auto&& part : accumulator) {
                ret.push_back(FN.Predicate->getEqualityPredicate(
                    part,
                    FN.Term->getTrueTerm(),
                    pred->getLocation(),
                    pred->getType()
                ));
            }
        } else {
            ret.push_back(r);
        }
    }
    if(ret != basic->getData()) return FN.State->Basic(std::move(ret));
    else return basic;
}

bool Normalizer::handleAnd(Term::Ptr term) {
    using namespace functional_hell::matchers::placeholders;
    SWITCH(term) {
        NAMED_CASE(m, $BinaryTerm(llvm::ArithType::LAND, _1,_2)) {
            if(not handleAnd(m->_1)) {
                accumulator.push_back(m->_1);
            }
            if(not handleAnd(m->_2)) {
                accumulator.push_back(m->_2);
            }
            return true;
        }
    }
    return false;
}

} /* namespace borealis */


