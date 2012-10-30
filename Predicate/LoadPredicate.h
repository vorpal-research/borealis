/*
 * LoadPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef LOADPREDICATE_H_
#define LOADPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"
#include "slottracker.h"

namespace borealis {

class LoadPredicate: public Predicate {

public:

	LoadPredicate(
			const llvm::Value* lhv,
			const llvm::Value* rhv,
			SlotTracker* st);
	virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

	virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

private:

	const std::string _lhv;
	const std::string _rhv;

	const llvm::Value* lhv;
	const llvm::Value* rhv;

};

} /* namespace borealis */

#endif /* LOADPREDICATE_H_ */
