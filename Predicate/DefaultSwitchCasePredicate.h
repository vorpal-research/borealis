/*
 * DefaultSwitchCasePredicate.h
 *
 *  Created on: Jan 17, 2013
 *      Author: ice-phoenix
 */

#ifndef DEFAULTSWITCHCASEPREDICATE_H_
#define DEFAULTSWITCHCASEPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class DefaultSwitchCasePredicate: public borealis::Predicate {

    typedef DefaultSwitchCasePredicate Self;

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const override;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<Self>();
    }

    static bool classof(const Self* /* p */) {
        return true;
    }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        std::vector<Term::Ptr> new_cases;
        new_cases.reserve(cases.size());
        std::transform(cases.begin(), cases.end(), std::back_inserter(new_cases),
            [t](const Term::Ptr& e) { return t->transform(e); }
        );

        return new Self(
            t->transform(cond),
            new_cases,
            this->type
        );
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

    virtual Predicate* clone() const override {
        return new Self{ *this };
    }

    friend class PredicateFactory;

private:

    Term::Ptr cond;
    const std::vector<Term::Ptr> cases;

    DefaultSwitchCasePredicate(
            Term::Ptr cond,
            std::vector<Term::Ptr> cases,
            PredicateType type = PredicateType::PATH);
    DefaultSwitchCasePredicate(const Self&) = default;

};

} /* namespace borealis */

#endif /* DEFAULTSWITCHCASEPREDICATE_H_ */
