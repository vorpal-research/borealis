/*
 * AggregateTransformer.h
 *
 *  Created on: Jul 12, 2013
 *      Author: ice-phoenix
 */

#ifndef AGGREGATETRANSFORMER_H_
#define AGGREGATETRANSFORMER_H_

#include "State/Transformer/Transformer.hpp"
#include "Util/macros.h"

namespace borealis {

template<class First, class Rest, class = GUARD(
    std::is_base_of<Transformer<util::decay_t<First>>, util::decay_t<First>>::value &&
    std::is_base_of<Transformer<util::decay_t<Rest>>, util::decay_t<Rest>>::value
)>
class AggregateTransformer : public borealis::Transformer<AggregateTransformer<First, Rest>> {

    typedef borealis::Transformer<AggregateTransformer<First, Rest>> Base;

public:

    AggregateTransformer(First&& first, Rest&& rest) :
        Base(FactoryNest()), // XXX: Placeholder FactoryNest.
                             //      Should not be used. Ever.
        first(std::forward<First>(first)),
        rest(std::forward<Rest>(rest)) {}

    PredicateState::Ptr transformBase(PredicateState::Ptr ps) {
        auto pps = first.transform(ps);
        return rest.transform(pps);
    }

    Predicate::Ptr transformBase(Predicate::Ptr p) {
        auto pp = first.transform(p);
        return rest.transform(pp);
    }

    Term::Ptr transformBase(Term::Ptr t) {
        auto tt = first.transform(t);
        return rest.transform(tt);
    }

    Annotation::Ptr transformBase(Annotation::Ptr a) {
        auto aa = first.transform(a);
        return rest.transform(aa);
    }

private:

    First first;
    Rest rest;

};

template<class First, class Rest>
AggregateTransformer<First, Rest> operator+(First&& first, Rest&& rest) {
    return AggregateTransformer<First, Rest>(
        std::forward<First>(first),
        std::forward<Rest>(rest)
    );
}

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* AGGREGATETRANSFORMER_H_ */
