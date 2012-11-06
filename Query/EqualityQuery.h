/*
 * EqualityQuery.h
 *
 *  Created on: Nov 6, 2012
 *      Author: ice-phoenix
 */

#ifndef EQUALITYQUERY_H_
#define EQUALITYQUERY_H_

#include <llvm/Value.h>

#include "Query.h"
#include "Util/slottracker.h"

namespace borealis {

class EqualityQuery {

public:

    EqualityQuery(const llvm::Value* v1,
            const llvm::Value* v2,
            SlotTracker* st);
    virtual ~EqualityQuery();
    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;
    virtual std::string toString() const;

private:

    const llvm::Value* v1;
    const llvm::Value* v2;

    const std::string _v1;
    const std::string _v2;

};

} /* namespace borealis */

#endif /* EQUALITYQUERY_H_ */
