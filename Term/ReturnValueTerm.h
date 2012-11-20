/*
 * ReturnValueTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef RETURNVALUETERM_H_
#define RETURNVALUETERM_H_

#include <llvm/Function.h>

#include "Term.h"
#include "Util/slottracker.h"

namespace borealis {

class TermFactory;

class ReturnValueTerm: public borealis::Term {

public:

    virtual ~ReturnValueTerm() {};

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ReturnValueTerm>();
    }

    static bool classof(const ReturnValueTerm* /* t */) {
        return true;
    }

    friend class TermFactory;

private:

    ReturnValueTerm(llvm::Function* F, SlotTracker* /* st */) :
        Term(
                (id_t)F,
                llvm::type2type(*F->getFunctionType()->getReturnType()),
                "\\result",
                type_id(*this)
            )
    {}

};

} /* namespace borealis */

#endif /* RETURNVALUETERM_H_ */
