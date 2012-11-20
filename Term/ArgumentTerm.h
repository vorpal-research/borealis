/*
 * ArgumentTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef ARGUMENTTERM_H_
#define ARGUMENTTERM_H_

#include "Term.h"
#include "Util/slottracker.h"

namespace borealis {

class TermFactory;

class ArgumentTerm: public borealis::Term {

public:

    virtual ~ArgumentTerm() {};

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ArgumentTerm>();
    }

    static bool classof(const ArgumentTerm* /* t */) {
        return true;
    }

    friend class TermFactory;

private:

    ArgumentTerm(const llvm::Value* v, SlotTracker* st) :
        Term((id_t)v, llvm::valueType(*v), st->getLocalName(v), type_id(*this))
    {}

};

} /* namespace borealis */

#endif /* ARGUMENTTERM_H_ */
