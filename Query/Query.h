/*
 * Query.h
 *
 *  Created on: Oct 11, 2012
 *      Author: ice-phoenix
 */

#ifndef QUERY_H_
#define QUERY_H_

#include <z3/z3++.h>

#include "Solver/Z3ExprFactory.h"
#include "Util/slottracker.h"

namespace borealis {

class Query {

public:

	virtual ~Query();
	virtual z3::expr toZ3(Z3ExprFactory& z3ef) const = 0;

};

} /* namespace borealis */

#endif /* QUERY_H_ */
