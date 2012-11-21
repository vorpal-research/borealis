/*
 * ArgumentTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef ARGUMENTTERM_H_
#define ARGUMENTTERM_H_

#include <llvm/Argument.h>

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

    llvm::Argument* getArgument() const {
        return a;
    }

    friend class TermFactory;

private:

    ArgumentTerm(llvm::Argument* a, SlotTracker* st) :
        Term((id_t)a, llvm::valueType(*a), st->getLocalName(a), type_id(*this))
    { this->a = a; }

    llvm::Argument* a;

};

} /* namespace borealis */

#endif /* ARGUMENTTERM_H_ */
