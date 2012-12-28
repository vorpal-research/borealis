/*
 * TermFactory.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef TERMFACTORY_H_
#define TERMFACTORY_H_

#include <llvm/Value.h>

#include <memory>

#include "Term/ArgumentTerm.h"
#include "Term/ConstTerm.h"
#include "Term/ReturnValueTerm.h"
#include "Term/Term.h"
#include "Term/ValueTerm.h"

#include "Util/slottracker.h"

namespace borealis {

class TermFactory {

public:

    typedef std::unique_ptr<TermFactory> Ptr;

    Term::Ptr getArgumentTerm(llvm::Argument* a) {
        return Term::Ptr(new ArgumentTerm(a, slotTracker));
    }

    Term::Ptr getConstTerm(llvm::Constant* c) {
        return Term::Ptr(new ConstTerm(c, slotTracker));
    }

    Term::Ptr getReturnValueTerm(llvm::Function* F) {
        return Term::Ptr(new ReturnValueTerm(F, slotTracker));
    }

    Term::Ptr getValueTerm(llvm::Value* v) {
        using namespace llvm;

        if (isa<ConstantPointerNull>(v) ||
                isa<ConstantInt>(v) ||
                isa<ConstantFP>(v))
            return getConstTerm(cast<Constant>(v));
        else
            return Term::Ptr(new ValueTerm(v, slotTracker));
    }

    static Ptr get(SlotTracker* slotTracker) {
        return Ptr(new TermFactory(slotTracker));
    }

private:

    SlotTracker* slotTracker;

    TermFactory(SlotTracker* slotTracker);

};

} /* namespace borealis */

#endif /* TERMFACTORY_H_ */
