//
// Created by ice-phoenix on 9/9/15.
//

#include "State/Transformer/Retyper.h"

#include "Util/macros.h"

namespace borealis {

Retyper::Retyper(FactoryNest FN) : Base(FN) {}

Term::Ptr Retyper::transformBase(Term::Ptr t) {
    Term::Ptr res = t;
    if (not types.empty() and TypeUtils::isUnknown(t->getType())) {
        res = t->setType(FN.Term.get(), types.top());
    }
    return Base::transformBase(res);
}

Term::Ptr Retyper::transformTerm(Term::Ptr t) {
    return t;
}

Term::Ptr Retyper::transformBinary(BinaryTermPtr t) {
    if (TypeUtils::isUnknown(t->getLhv()->getType()) and TypeUtils::isUnknown(t->getRhv()->getType())) {
        types.push(FN.Type->getInteger(64, llvm::Signedness::Unknown));
    } else if (TypeUtils::isConcrete(t->getLhv()->getType()) and TypeUtils::isUnknown(t->getRhv()->getType())) {
        types.push(t->getLhv()->getType());
    } else if (TypeUtils::isUnknown(t->getLhv()->getType()) and TypeUtils::isConcrete(t->getRhv()->getType())) {
        types.push(t->getRhv()->getType());
    } else {
        return Base::transformBinary(t);
    }
    ON_SCOPE_EXIT(types.pop());
    return Base::transformBinary(t);
}

Term::Ptr Retyper::transformCmp(CmpTermPtr t) {
    if (TypeUtils::isUnknown(t->getLhv()->getType()) and TypeUtils::isUnknown(t->getRhv()->getType())) {
        types.push(FN.Type->getInteger(64, llvm::Signedness::Unknown));
    } else if (TypeUtils::isConcrete(t->getLhv()->getType()) and TypeUtils::isUnknown(t->getRhv()->getType())) {
        types.push(t->getLhv()->getType());
    } else if (TypeUtils::isUnknown(t->getLhv()->getType()) and TypeUtils::isConcrete(t->getRhv()->getType())) {
        types.push(t->getRhv()->getType());
    } else {
        return Base::transformCmp(t);
    }
    ON_SCOPE_EXIT(types.pop());
    return Base::transformCmp(t);
}

Predicate::Ptr Retyper::transformSeqData(SeqDataPredicatePtr p) {
    return p->shared_from_this();
}

} // namespace borealis

#include "Util/unmacros.h"
