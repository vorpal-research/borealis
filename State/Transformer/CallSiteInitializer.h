/*
 * CallSiteInitializer.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef CALLSITEINITIALIZER_H_
#define CALLSITEINITIALIZER_H_

#include <llvm/Argument.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Value.h>

#include <unordered_map>

#include "Predicate/PredicateFactory.h"
#include "State/Transformer/Transformer.hpp"
#include "Term/TermFactory.h"

#include "Util/macros.h"

namespace borealis {

class CallSiteInitializer : public borealis::Transformer<CallSiteInitializer> {

public:

    CallSiteInitializer(
            llvm::CallInst& CI,
            TermFactory* TF) {

        using borealis::util::toString;

        this->returnValue = &CI;

        int argIdx = 0;
        for (auto& formal : CI.getCalledFunction()->getArgumentList()) {
            auto* actual = CI.getArgOperand(argIdx++);
            callSiteArguments[&formal] = actual;
        }

        auto* callerFunc = CI.getParent()->getParent();
        auto* callerInst = &CI;

        auto callerFuncName = callerFunc && callerFunc->hasName()
                              ? callerFunc->getName().str()
                              : toString(callerFunc);
        auto callerInstName = callerInst && callerInst->hasName()
                              ? callerInst->getName().str()
                              : toString(callerInst);

        prefix = callerFuncName + "." + callerInstName + ".";

        this->TF = TF;
    }

    Predicate::Ptr transformPredicate(Predicate::Ptr p) {
        switch(p->getType()) {
        case PredicateType::ENSURES:
            return Predicate::Ptr(
                p->clone()->setType(PredicateType::STATE)
            );
        default:
            return p;
        }
    }

    Term::Ptr transformArgumentTerm(ArgumentTermPtr t) {
        auto* formal = t->getArgument();

        ASSERT(callSiteArguments.count(formal) > 0,
               "Cannot find an actual function argument at call site");

        auto* actual = callSiteArguments[formal];

        return TF->getValueTerm(actual);
    }

    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr) {
        return TF->getValueTerm(returnValue);
    }

    Term::Ptr transformValueTerm(ValueTermPtr t) {
        auto renamed = prefix + t->getName();
        return t->withNewName(renamed);
    }

private:

    typedef std::unordered_map<llvm::Argument*, llvm::Value*> CallSiteArguments;

    llvm::Value* returnValue;
    CallSiteArguments callSiteArguments;
    std::string prefix;

    TermFactory* TF;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CALLSITEINITIALIZER_H_ */
