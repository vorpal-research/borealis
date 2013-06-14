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
            llvm::CallInst& I,
            TermFactory* TF) {

        auto& formalArgs = I.getCalledFunction()->getArgumentList();
        int argIdx = 0;

        this->returnValue = &I;
        for (auto& formal : formalArgs) {
            auto* actual = I.getArgOperand(argIdx++);
            callSiteArguments[&formal] = actual;
        }

        this->TF = TF;
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

private:

    typedef std::unordered_map<llvm::Argument*, llvm::Value*> CallSiteArguments;

    llvm::Value* returnValue;
    CallSiteArguments callSiteArguments;

    TermFactory* TF;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CALLSITEINITIALIZER_H_ */
