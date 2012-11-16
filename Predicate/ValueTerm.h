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

class ValueTerm: public borealis::Term {

public:

    ValueTerm(llvm::Value* v, SlotTracker* st) :
        Term(llvm::valueType(*v), st->getLocalName(v))
    {}
    virtual ~ValueTerm() {};

};

} /* namespace borealis */

#endif /* VALUETERM_H_ */
