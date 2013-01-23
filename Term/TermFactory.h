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
#include "Term/BinaryTerm.h"
#include "Term/UnaryTerm.h"
#include "Term/CmpTerm.h"
#include "Term/OpaqueBuiltinTerm.h"
#include "Term/OpaqueFloatingConstantTerm.h"
#include "Term/OpaqueIntConstantTerm.h"
#include "Term/OpaqueBoolConstantTerm.h"
#include "Term/OpaqueVarTerm.h"

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
        else if(auto* arg = dyn_cast<Argument>(v))
            return getArgumentTerm(arg);
        else
            return Term::Ptr(new ValueTerm(v, slotTracker));
    }

    Term::Ptr getBinaryTerm(llvm::ArithType opc, Term::Ptr lhv, Term::Ptr rhv) {
        return Term::Ptr(new BinaryTerm(opc, lhv, rhv));
    }

    Term::Ptr getUnaryTerm(llvm::UnaryArithType opc, Term::Ptr rhv) {
        return Term::Ptr(new UnaryTerm(opc, rhv));
    }

    Term::Ptr getCmpTerm(llvm::ConditionType opc, Term::Ptr lhv, Term::Ptr rhv) {
        return Term::Ptr(new CmpTerm(opc, lhv, rhv));
    }

    Term::Ptr getOpaqueVarTerm(const std::string& name) {
        return Term::Ptr(new OpaqueVarTerm(name));
    }

    Term::Ptr getOpaqueBuiltinTerm(const std::string& name) {
        return Term::Ptr(new OpaqueBuiltinTerm(name));
    }

    Term::Ptr getOpaqueConstantTerm(long long v) {
        return Term::Ptr(new OpaqueIntConstantTerm(v));
    }

    Term::Ptr getOpaqueConstantTerm(double v) {
        return Term::Ptr(new OpaqueFloatingConstantTerm(v));
    }

    Term::Ptr getOpaqueConstantTerm(bool v) {
        return Term::Ptr(new OpaqueBoolConstantTerm(v));
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
