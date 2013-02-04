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
#include "Term/TermFactory.h"

#include "State/Transformer.hpp"

namespace borealis {

class CallSiteInitializer : public borealis::Transformer<CallSiteInitializer> {

public:

    CallSiteInitializer(
            llvm::CallInst& I,
            TermFactory* TF) {
        using llvm::Argument;
        using llvm::Value;

        auto& formalArgs = I.getCalledFunction()->getArgumentList();
        int argIdx = 0;

        this->returnValue = &I;
        for (Argument& formal : formalArgs) {
            Value* actual = I.getArgOperand(argIdx++);
            callSiteArguments[&formal] = actual;
        }

        this->TF = TF;
    }

    Term::Ptr transformArgumentTerm(ArgumentTermPtr t) {
        using llvm::Argument;
        using llvm::Value;

        Argument* formal = t->getArgument();
        Value* actual = callSiteArguments[formal];

        return TF->getValueTerm(actual);
    }

    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr /* t */) {
        return TF->getValueTerm(returnValue);
    }

private:

    typedef std::unordered_map<llvm::Argument*, llvm::Value*> CallSiteArguments;

    llvm::Value* returnValue;
    CallSiteArguments callSiteArguments;

    TermFactory* TF;

};

} /* namespace borealis */

#endif /* CALLSITEINITIALIZER_H_ */
