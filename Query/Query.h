/*
 * Query.h
 *
 *  Created on: Oct 11, 2012
 *      Author: ice-phoenix
 */

#ifndef QUERY_H_
#define QUERY_H_

#include <z3/z3++.h>

namespace borealis {

class Query {

public:

	virtual ~Query();
	virtual z3::expr toZ3(z3::context& ctx) const = 0;

};

} /* namespace borealis */

#endif /* QUERY_H_ */
