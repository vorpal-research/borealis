/*
 * AllocaPredicate.h
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#ifndef ALLOCAPREDICATE_H_
#define ALLOCAPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/AllocaPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message AllocaPredicate {
    extend borealis.proto.Predicate {
        optional AllocaPredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term numElements = 2;
    optional Term origNumElements = 3;
}

**/
class AllocaPredicate: public borealis::Predicate {

    Term::Ptr lhv;
    Term::Ptr numElements;
    Term::Ptr origNumElements;

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            Term::Ptr origNumElements,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(AllocaPredicate);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getNumElems() const { return numElements; }
    Term::Ptr getOrigNumElems() const { return origNumElements; }

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto _lhv = t->transform(lhv);
        auto _numElements = t->transform(numElements);
        auto _origNumElements = t->transform(origNumElements);
        auto _loc = location;
        auto _type = type;
        PREDICATE_ON_CHANGED(
            lhv != _lhv || numElements != _numElements || origNumElements != _origNumElements,
            new Self( _lhv, _numElements, _origNumElements, _loc, _type )
        );
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, AllocaPredicate> {
    static Bool<Impl> doit(
            const AllocaPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto lhve = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(!lhve.empty(), "Encountered alloca with non-Pointer left side");
        auto lhvp = lhve.getUnsafe();

        unsigned long long elems = 1;
        if (auto* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(p->getNumElems())) {
            elems = cnst->getValue();
        } else {
            BYE_BYE(Bool, "Encountered alloca with non-integer element number");
        }

        auto origSize = SMT<Impl>::doit(p->getOrigNumElems(), ef, ctx).template to<Integer>();
        ASSERT(!origSize.empty(), "Encountered alloca with non-integer original size");
        auto origSizeInt = origSize.getUnsafe();

        return lhvp == ctx->getLocalPtr(elems, origSizeInt);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */
