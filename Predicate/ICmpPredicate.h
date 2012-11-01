/*
 * ICmpPredicate.hDEREF_VALUE
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef ICMPPREDICATE_H_
#define ICMPPREDICATE_H_

#include <llvm/Value.h>

#include "Predicate.h"

namespace borealis {

class ICmpPredicate: public Predicate {

public:

    ICmpPredicate(
            const llvm::Value* lhv,
            const llvm::Value* op1,
            const llvm::Value* op2,
            const int cond,
            SlotTracker* st);
    virtual Predicate::Key getKey() const;

    virtual Dependee getDependee() const;
    virtual DependeeSet getDependees() const;

    virtual z3::expr toZ3(Z3ExprFactory& z3ef) const;

private:

    const llvm::Value* lhv;
    const llvm::Value* op1;
    const llvm::Value* op2;
    const int cond;

    const std::string _lhv;
    const std::string _op1;
    const std::string _op2;
    const std::string _cond;

};

} /* namespace borealis */

#endif /* ICMPPREDICATE_H_ */
