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

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            Term::Ptr origNumElements,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(AllocaPredicate);

    Term::Ptr getLhv() const;
    Term::Ptr getNumElems() const;
    Term::Ptr getOrigNumElems() const;

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _lhv = t->transform(getLhv());
        auto&& _numElements = t->transform(getNumElems());
        auto&& _origNumElements = t->transform(getOrigNumElems());
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getLhv() != _lhv || getNumElems() != _numElements || getOrigNumElems() != _origNumElements,
            new Self( _lhv, _numElements, _origNumElements, _loc, _type )
        );
    }

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

        auto&& lhve = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(not lhve.empty(), "Encountered alloca with non-Pointer left side");
        auto&& lhvp = lhve.getUnsafe();

        auto&& elems = 1ULL;
        if (auto* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(p->getNumElems())) {
            elems = cnst->getValue();
        } else {
            BYE_BYE(Bool, "Encountered alloca with non-integer element number");
        }

        auto&& origSize = SMT<Impl>::doit(p->getOrigNumElems(), ef, ctx).template to<Integer>();
        ASSERT(not origSize.empty(), "Encountered alloca with non-integer original size");
        auto&& origSizeInt = origSize.getUnsafe();

        return lhvp == ctx->getLocalPtr(elems, origSizeInt);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */
