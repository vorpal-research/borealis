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

class ArgumentTerm: public borealis::Term {

public:

    ArgumentTerm(const llvm::Value* v, SlotTracker* st) :
        Term((id_t)v, llvm::valueType(*v), st->getLocalName(v), type_id(*this))
    {}
    virtual ~ArgumentTerm() {};

};

} /* namespace borealis */

#endif /* ARGUMENTTERM_H_ */
