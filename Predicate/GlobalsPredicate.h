/*
 * GlobalsPredicate.h
 *
 *  Created on: Mar 13, 2013
 *      Author: ice-phoenix
 */

#ifndef GLOBALSPREDICATE_H_
#define GLOBALSPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

class GlobalsPredicate: public borealis::Predicate {

public:

    virtual logic::Bool toZ3(Z3ExprFactory& z3ef, ExecutionContext*) const override;

    static bool classof(const Predicate* p) {
        return p->getPredicateTypeId() == type_id<GlobalsPredicate>();
    }

    static bool classof(const GlobalsPredicate* /* p */) {
        return true;
    }

    template<class SubClass>
    const GlobalsPredicate* accept(Transformer<SubClass>* t) const {
        std::vector<Term::Ptr> new_globals;
        new_globals.reserve(globals.size());
        std::transform(globals.begin(), globals.end(), std::back_inserter(new_globals),
            [t](const Term::Ptr& e) { return t->transform(e); }
        );

        return new GlobalsPredicate(
                new_globals,
                this->type);
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

    friend class PredicateFactory;

private:

    const std::vector<Term::Ptr> globals;

    GlobalsPredicate(
            const std::vector<Term::Ptr>& globals,
            PredicateType type = PredicateType::STATE);

};

} /* namespace borealis */

#endif /* GLOBALSPREDICATE_H_ */
