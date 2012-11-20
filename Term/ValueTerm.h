/*
 * ValueTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef VALUETERM_H_
#define VALUETERM_H_

#include <llvm/Value.h>

#include "Term.h"
#include "Util/slottracker.h"

namespace borealis {

class TermFactory;
class GEPPredicate;

class ValueTerm: public borealis::Term {

public:

    virtual ~ValueTerm() {};

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ValueTerm>();
    }

    static bool classof(const ValueTerm* /* t */) {
        return true;
    }

    friend class TermFactory;
    friend class GEPPredicate;

private:

    ValueTerm(const llvm::Value* v, SlotTracker* st) :
        Term((id_t)v, llvm::valueType(*v), st->getLocalName(v), type_id(*this))
    {}

};

} /* namespace borealis */

#endif /* VALUETERM_H_ */
