/*
 * NullPtrQuery.h
 *
 *  Created on: Oct 11, 2012
 *      Author: ice-phoenix
 */

#ifndef NULLPTRQUERY_H_
#define NULLPTRQUERY_H_

#include <llvm/Value.h>

#include "Query.h"
#include "../slottracker.h"

namespace borealis {

class NullPtrQuery : public Query {

public:

	NullPtrQuery(const llvm::Value* ptr, SlotTracker* st);
	virtual ~NullPtrQuery();
	virtual z3::expr toZ3(z3::context& ctx) const;

private:

	const llvm::Value* ptr;
	const std::string ptrs;

};

} /* namespace borealis */

#endif /* NULLPTRQUERY_H_ */
