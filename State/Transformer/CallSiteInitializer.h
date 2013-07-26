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

#include "State/Transformer/Transformer.hpp"

#include "Util/macros.h"

namespace borealis {

class CallSiteInitializer : public borealis::Transformer<CallSiteInitializer> {

    typedef borealis::Transformer<CallSiteInitializer> Base;

public:

    CallSiteInitializer(
            llvm::CallInst& CI,
            FactoryNest FN) : Base(FN) {

        using borealis::util::toString;

        returnValue = &CI;

        int argNum = CI.getCalledFunction()->getArgumentList().size();
        for (int argIdx = 0; argIdx < argNum; ++argIdx) {
            callSiteArguments[argIdx] = CI.getArgOperand(argIdx);
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
        auto argIdx = t->getIdx();

        ASSERT(callSiteArguments.count(argIdx) > 0,
               "Cannot find an actual function argument at call site");

        auto* actual = callSiteArguments.at(argIdx);

        return FN.Term->getValueTerm(actual);
    }

    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr) {
        return FN.Term->getValueTerm(returnValue);
    }

    Term::Ptr transformValueTerm(ValueTermPtr t) {
        auto renamed = prefix + t->getName();
        return t->withNewName(renamed);
    }

private:

    typedef std::unordered_map<unsigned int, llvm::Value*> CallSiteArguments;

    llvm::Value* returnValue;
    CallSiteArguments callSiteArguments;
    std::string prefix;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CALLSITEINITIALIZER_H_ */
