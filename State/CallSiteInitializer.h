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

#include "Transformer.hpp"

namespace borealis {

class CallSiteInitializer : public Transformer<CallSiteInitializer> {

public:

    CallSiteInitializer(
            llvm::CallInst& I,
            PredicateFactory* PF,
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

        this->PF = PF;
        this->TF = TF;
    }

    const Term* transformArgumentTerm(const ArgumentTerm* t) {
        using llvm::Argument;
        using llvm::Value;

        Argument* formal = t->getArgument();
        Value* actual = callSiteArguments[formal];

        return TF->getValueTerm(actual).release();
    }

    const Term* transformConstTerm(const ConstTerm* t) {
        return TF->getConstTerm(t->getType(), t->getName()).release();
    }

    const Term* transformReturnValueTerm(const ReturnValueTerm* /* t */) {
        return TF->getValueTerm(returnValue).release();
    }

    const Term* transformValueTerm(const ValueTerm* t) {
        return TF->getValueTerm(t->getValue()).release();
    }

private:

    typedef std::unordered_map<llvm::Argument*, llvm::Value*> CallSiteArguments;

    llvm::Value* returnValue;
    CallSiteArguments callSiteArguments;

    PredicateFactory* PF;
    TermFactory* TF;

};

} /* namespace borealis */

#endif /* CALLSITEINITIALIZER_H_ */
