/*
 * PredicateStateBuilder.h
 *
 *  Created on: May 23, 2013
 *      Author: ice-phoenix
 */

#ifndef PREDICATESTATEBUILDER_H_
#define PREDICATESTATEBUILDER_H_

#include "State/PredicateStateFactory.h"

namespace borealis {

class PredicateStateBuilder {

private:

    PredicateStateFactory::Ptr PSF;
    PredicateState::Ptr State;

public:

    PredicateStateBuilder(PredicateStateFactory::Ptr PSF, PredicateState::Ptr state);
    PredicateStateBuilder(PredicateStateFactory::Ptr PSF, Predicate::Ptr pred);
    PredicateStateBuilder(const PredicateStateBuilder&) = default;
    PredicateStateBuilder(PredicateStateBuilder&&) = default;

    PredicateState::Ptr operator()() const;

    PredicateState::Ptr apply() const;

    template<class TT>
    PredicateState::Ptr with(TT&& t) const {
        return std::forward<TT>(t).transform(State);
    }

    PredicateStateBuilder& operator+=(PredicateState::Ptr s);
    PredicateStateBuilder& operator+=(Predicate::Ptr p);
    PredicateStateBuilder& operator<<=(const Locus& locus);

    friend PredicateStateBuilder operator+ (PredicateStateBuilder PSB, PredicateState::Ptr s);
    friend PredicateStateBuilder operator+ (PredicateStateBuilder PSB, Predicate::Ptr p);
    friend PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const Locus& locus);

};

PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, PredicateState::Ptr s);
PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, Predicate::Ptr p);

template<class Container>
PredicateStateBuilder operator*(PredicateStateFactory::Ptr PSF, Container&& c) {
    PredicateStateBuilder res{PSF, PSF->Basic()};
    for (auto&& p : c) {
        res += p;
    }
    return res;
}

PredicateStateBuilder operator+ (PredicateStateBuilder PSB, PredicateState::Ptr s);
PredicateStateBuilder operator+ (PredicateStateBuilder PSB, Predicate::Ptr p);

template<class Container>
PredicateStateBuilder operator+(PredicateStateBuilder PSB, Container&& c) {
    PredicateStateBuilder res{PSB};
    for (auto&& p : c) {
        res += p;
    }
    return res;
}

PredicateStateBuilder operator<<(PredicateStateBuilder PSB, const Locus& locus);

template<class Container>
PredicateStateBuilder operator<<(PredicateStateBuilder PSB, Container&& loci) {
    PredicateStateBuilder res{PSB};
    for (auto&& locus : loci) {
        res <<= locus;
    }
    return res;
}

} /* namespace borealis */

#endif /* PREDICATESTATEBUILDER_H_ */
