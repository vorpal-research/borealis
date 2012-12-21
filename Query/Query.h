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
#include "Util/util.h"

namespace borealis {

class Query {

public:

	virtual ~Query() = 0;
	virtual logic::Bool toZ3(Z3ExprFactory& z3ef) const = 0;
	virtual std::string toString() const = 0;

};

llvm::raw_ostream& operator<<(llvm::raw_ostream& s, const borealis::Query& q);
std::ostream& operator<<(std::ostream& s, const borealis::Query& q);

} /* namespace borealis */

#endif /* QUERY_H_ */
