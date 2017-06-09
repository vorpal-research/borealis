/*
 * CallSiteInitializer.cpp
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#include "State/Transformer/CallSiteInitializer.h"

#include "Util/macros.h"

#define LASSERT(X, ...) { if(not (X)) failWith(__VA_ARGS__); }
#define LASSERTC(...) LASSERT(static_cast<bool>(__VA_ARGS__), #__VA_ARGS__)

namespace borealis {

CallSiteInitializer::CallSiteInitializer(
        llvm::ImmutableCallSite CI,
        FactoryNest FN) : CallSiteInitializer(CI, FN, nullptr){}

CallSiteInitializer::CallSiteInitializer(
        llvm::ImmutableCallSite CI,
        FactoryNest FN,
        const Locus* loc) : Base(FN), ci(CI) {

    using borealis::util::toString;

    auto* callerInst = CI.getInstruction();
    auto* callerFunc = callerInst->getParent()->getParent();

    auto&& callerFuncName = callerFunc && callerFunc->hasName()
                            ? callerFunc->getName().str()
                            : toString(callerFunc);
    auto&& callerInstName = callerInst && callerInst->hasName()
                            ? callerInst->getName().str()
                            : tfm::format("%%%d", FN.Slot->getLocalSlot(callerInst));

    prefix = callerFuncName + "." + callerInstName + ".";
    if(loc) overrideLoc = loc;
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

Annotation::Ptr CallSiteInitializer::transformRequiresAnnotation(RequiresAnnotationPtr p) {
    auto loc = overrideLoc? *overrideLoc : borealis::Locus(ci->getDebugLoc());
    return AssertAnnotation::fromTerms(loc, p->getMeta(), util::make_vector(p->getTerm()));
}

Annotation::Ptr CallSiteInitializer::transformEnsuresAnnotation(EnsuresAnnotationPtr p) {
    auto loc = overrideLoc? *overrideLoc : borealis::Locus(ci->getDebugLoc());
    return AssumeAnnotation::fromTerms(loc, p->getMeta(), util::make_vector(p->getTerm()));
}

Annotation::Ptr CallSiteInitializer::transformGlobalAnnotation(GlobalAnnotationPtr p) {
    auto loc = overrideLoc? *overrideLoc : borealis::Locus(ci->getDebugLoc());
    return AssumeAnnotation::fromTerms(loc, p->getMeta(), util::make_vector(p->getTerm()));
}

Term::Ptr CallSiteInitializer::transformArgumentTerm(ArgumentTermPtr t) {
    auto argIdx = t->getIdx();

    LASSERT(ci.arg_size() > argIdx,
            "Cannot find an actual function argument at call site: " +
            llvm::valueSummary(ci.getInstruction()));

    auto* actual = ci.getArgument(argIdx);

    switch (t->getKind()) {
    case ArgumentKind::STRING: {
        auto&& argAsString = getAsCompileTimeString(actual);
        LASSERT(not argAsString.empty(),
                "Non-string actual argument for ArgumentKind::STRING");
        return FN.Term->getOpaqueConstantTerm(*argAsString.get());
    }
    default:
        return FN.Term->getValueTerm(actual);
    }
}

Term::Ptr CallSiteInitializer::transformArgumentCountTerm(ArgumentCountTermPtr /*t*/) {
    return FN.Term->getIntTerm(ci.arg_size(), TypeFactory::defaultTypeSize);
}

Term::Ptr CallSiteInitializer::transformVarArgumentTerm(VarArgumentTermPtr t) {
    auto argIdx = t->getIdx() + ci.getCalledFunction()->arg_size();

    LASSERT(ci.arg_size() > argIdx,
            "Cannot find an actual function argument at call site: %s",
            llvm::valueSummary(ci.getInstruction()));

    auto* actual = ci.getArgument(argIdx);

    return FN.Term->getValueTerm(actual);
}

Term::Ptr CallSiteInitializer::transformReturnValueTerm(ReturnValueTermPtr) {
    return FN.Term->getValueTerm(ci.getInstruction());
}

Term::Ptr CallSiteInitializer::transformReturnPtrTerm(ReturnPtrTermPtr) {
    auto&& binder = util::viewContainer(ci.getInstruction()->uses())
                          .map(llvm::ops::dyn_cast<llvm::StoreInst>)
                          .filter()
                          .first_or(nullptr);
    LASSERTC(binder);
    auto&& realPtr = binder->getPointerOperand();

    return FN.Term->getValueTerm(realPtr);
}

Term::Ptr CallSiteInitializer::transformValueTerm(ValueTermPtr t) {
    if (t->isGlobal()) return t;
    else return t->withNewName(prefix + t->getName());
}

} /* namespace borealis */

#include "Util/unmacros.h"
