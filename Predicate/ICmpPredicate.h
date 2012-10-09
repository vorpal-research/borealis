/*
 * ICmpPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef ICMPPREDICATE_H_
#define ICMPPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

#include "../slottracker.h"

namespace borealis {

class ICmpPredicate: public Predicate {

public:

	ICmpPredicate(
			const llvm::Value* lhv,
			const llvm::Value* op1,
			const llvm::Value* op2,
			const int cond,
			SlotTracker* st);
	virtual std::string toString() const;
	virtual Predicate::Key getKey() const;
	virtual z3::expr toZ3(z3::context& ctx) const;

private:

	const llvm::Value* lhv;
	const llvm::Value* op1;
	const llvm::Value* op2;
	const int cond;

	const std::string lhvs;
	const std::string op1s;
	const std::string op2s;
	const std::string conds;

	const std::string asString;

};

} /* namespace borealis */

#endif /* ICMPPREDICATE_H_ */
