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
#include "slottracker.h"

namespace borealis {

class GEPPredicate: public Predicate {

public:

    GEPPredicate(
            const llvm::Value* lhv,
            const llvm::Value* rhv,
            const std::vector< std::pair<const llvm::Value*, uint64_t> > shifts,
            SlotTracker* st);

    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(z3::context& ctx) const;

private:

    const llvm::Value* lhv;
    const llvm::Value* rhv;
    std::vector< std::tuple<const llvm::Value*, std::string, uint64_t> > shifts;

    const std::string _lhv;
    const std::string _rhv;

};

} /* namespace borealis */

#endif /* GEPPREDICATE_H_ */
