/*
 * TermFactory.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef TERMFACTORY_H_
#define TERMFACTORY_H_

#include <llvm/Value.h>

#include "Term/ArgumentTerm.h"
#include "Term/ConstTerm.h"
#include "Term/ReturnValueTerm.h"
#include "Term/Term.h"
#include "Term/ValueTerm.h"

#include "Util/slottracker.h"

namespace borealis {

class TermFactory {

public:

    Term::Ptr getArgumentTerm(llvm::Argument* v) {
        return Term::Ptr(new ArgumentTerm(v, slotTracker));
    }

    Term::Ptr getConstTerm(llvm::ValueType type, const std::string& name) {
        return Term::Ptr(new ConstTerm(type, name));
    }

    Term::Ptr getReturnValueTerm(llvm::Function* f) {
        return Term::Ptr(new ReturnValueTerm(f, slotTracker));
    }

    Term::Ptr getValueTerm(llvm::Value* v) {
        return Term::Ptr(new ValueTerm(v, slotTracker));
    }

    static std::unique_ptr<TermFactory> get(SlotTracker* slotTracker) {
        return std::unique_ptr<TermFactory>(new TermFactory(slotTracker));
    }

private:

    SlotTracker* slotTracker;

    TermFactory(SlotTracker* slotTracker);

};

} /* namespace borealis */

#endif /* TERMFACTORY_H_ */
