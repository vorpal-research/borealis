/*
 * GEPPredicate.h
 *
 *  Created on: Oct 23, 2012
 *      Author: ice-phoenix
 */

#ifndef GEPPREDICATE_H_
#define GEPPREDICATE_H_

#include <tuple>

#include "Predicate.h"

namespace borealis {

class GEPPredicate: public Predicate {

public:

    GEPPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const std::vector< std::pair<const llvm::Value*, uint64_t> > shifts,
            SlotTracker* st);

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

private:

    const Term::Ptr lhv;
    const Term::Ptr rhv;
    std::vector< std::pair< Term::Ptr, Term::Ptr > > shifts;

};

} /* namespace borealis */

#endif /* GEPPREDICATE_H_ */
