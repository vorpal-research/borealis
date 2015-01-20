/*
 * CallSiteInitializer.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef CALLSITEINITIALIZER_H_
#define CALLSITEINITIALIZER_H_

#include <llvm/IR/Argument.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <unordered_map>

#include "State/Transformer/Transformer.hpp"

#include "Util/macros.h"

namespace borealis {

class CallSiteInitializer : public borealis::Transformer<CallSiteInitializer> {

    typedef borealis::Transformer<CallSiteInitializer> Base;

public:

    CallSiteInitializer(
            const llvm::CallInst& CI,
            FactoryNest FN) : Base(FN) {

        using borealis::util::toString;

        returnValue = &CI;

        if (auto* calledFunc = CI.getCalledFunction()) {
            int argNum = calledFunc->arg_size();
            for (int argIdx = 0; argIdx < argNum; ++argIdx) {
                callSiteArguments[argIdx] = CI.getArgOperand(argIdx);
            }
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

        switch (t->getKind()) {
        case ArgumentKind::STRING: {
            auto argAsString = getAsCompileTimeString(actual);
            ASSERT(!argAsString.empty(),
                   "Non-string actual argument for ArgumentKind::STRING");
            return FN.Term->getOpaqueConstantTerm(*argAsString.get());
        }
        default: {
            return FN.Term->getValueTerm(actual);
        }
        }
    }

    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr) {
        return FN.Term->getValueTerm(returnValue);
    }

    Term::Ptr transformValueTerm(ValueTermPtr t) {
        if (t->isGlobal()) return t;

        return t->withNewName(prefix + t->getName());
    }

private:

    typedef std::unordered_map<unsigned int, const llvm::Value*> CallSiteArguments;

    const llvm::Value* returnValue;
    CallSiteArguments callSiteArguments;
    std::string prefix;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CALLSITEINITIALIZER_H_ */
