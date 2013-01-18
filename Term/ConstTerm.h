/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include "Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ConstTerm: public borealis::Term {

public:

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ConstTerm>();
    }

    static bool classof(const ConstTerm* /* t */) {
        return true;
    }

    llvm::Constant* getConstant() const {
        return constant;
    }

    friend class PredicateFactory;
    friend class TermFactory;

    ConstTerm(const ConstTerm&) = default;
private:

    ConstTerm(llvm::Constant* c, SlotTracker* st) :
        Term((id_t)c, llvm::valueType(*c), st->getLocalName(c), type_id(*this)),
        constant(c)
    {}

    llvm::Constant* constant;

};

} /* namespace borealis */

#endif /* CONSTTERM_H_ */
