/*
 * AndQuery.h
 *
 *  Created on: Nov 6, 2012
 *      Author: ice-phoenix
 */

#ifndef ANDQUERY_H_
#define ANDQUERY_H_

#include <initializer_list>

#include "Query.h"

namespace borealis {

class AndQuery : public Query {

public:

    AndQuery(std::initializer_list<Query*> qs);
    virtual ~AndQuery();
    virtual logic::Bool toZ3(Z3ExprFactory& z3ef) const;
    virtual std::string toString() const;

private:

    const std::vector<Query*> qs;

};

} /* namespace borealis */

#endif /* ANDQUERY_H_ */
