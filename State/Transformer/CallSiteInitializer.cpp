/*
 * CallSiteInitializer.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#include "State/Transformer/CallSiteInitializer.h"

#include "Util/macros.h"

namespace borealis {

CallSiteInitializer::CallSiteInitializer(
        const llvm::CallInst& CI,
        FactoryNest FN) : Base(FN) {

    using borealis::util::toString;

    returnValue = &CI;

    if (auto* calledFunc = CI.getCalledFunction()) {
        auto&& argNum = calledFunc->arg_size();
        for (auto&& argIdx = 0U; argIdx < argNum; ++argIdx) {
            callSiteArguments[argIdx] = CI.getArgOperand(argIdx);
        }
    }

    auto* callerFunc = CI.getParent()->getParent();
    auto* callerInst = &CI;

    auto&& callerFuncName = callerFunc && callerFunc->hasName()
                            ? callerFunc->getName().str()
                            : toString(callerFunc);
    auto&& callerInstName = callerInst && callerInst->hasName()
                            ? callerInst->getName().str()
                            : toString(callerInst);

    prefix = callerFuncName + "." + callerInstName + ".";
}

Predicate::Ptr CallSiteInitializer::transformPredicate(Predicate::Ptr p) {
    switch(p->getType()) {
    case PredicateType::ENSURES:
        return Predicate::Ptr(
            p->clone()->setType(PredicateType::ASSUME)
        );
    default:
        return p;
    }
}

Term::Ptr CallSiteInitializer::transformArgumentTerm(ArgumentTermPtr t) {
    auto argIdx = t->getIdx();

    ASSERT(callSiteArguments.count(argIdx) > 0,
           "Cannot find an actual function argument at call site: " +
           llvm::valueSummary(returnValue));

    auto* actual = callSiteArguments.at(argIdx);

    switch (t->getKind()) {
    case ArgumentKind::STRING: {
        auto&& argAsString = getAsCompileTimeString(actual);
        ASSERT(not argAsString.empty(),
               "Non-string actual argument for ArgumentKind::STRING");
        return FN.Term->getOpaqueConstantTerm(*argAsString.get());
    }
    default:
        return FN.Term->getValueTerm(actual);
    }
}

Term::Ptr CallSiteInitializer::transformReturnValueTerm(ReturnValueTermPtr) {
    return FN.Term->getValueTerm(returnValue);
}

Term::Ptr CallSiteInitializer::transformValueTerm(ValueTermPtr t) {
    if (t->isGlobal()) return t;
    else return t->withNewName(prefix + t->getName());
}

} /* namespace borealis */

#include "Util/unmacros.h"
