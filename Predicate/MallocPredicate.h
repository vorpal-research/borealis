/*
 * MallocPredicate.h
 *
 *  Created on: Nov 30, 2012
 *      Author: ice-phoenix
 */

#ifndef MALLOCPREDICATE_H_
#define MALLOCPREDICATE_H_

#include "Config/config.h"
#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/MallocPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message MallocPredicate {
    extend borealis.proto.Predicate {
        optional MallocPredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term numElements = 2;
}

**/
class MallocPredicate: public borealis::Predicate {

    Term::Ptr lhv;
    Term::Ptr numElements;

    MallocPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(MallocPredicate);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getNumElems() const { return numElements; }

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto _lhv = t->transform(lhv);
        auto _numElements = t->transform(numElements);
        auto _loc = location;
        auto _type = type;
        PREDICATE_ON_CHANGED(
            lhv != _lhv || numElements != _numElements,
            new Self( _lhv, _numElements, _loc, _type )
        );
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, MallocPredicate> {
    static Bool<Impl> doit(
            const MallocPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto lhve = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(!lhve.empty(), "Malloc produces a non-pointer");
        auto lhvp = lhve.getUnsafe();

        unsigned long long elems = 1;
        if (auto* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(p->getNumElems())) {
            elems = cnst->getValue();
        } else {
            BYE_BYE(Bool, "Encountered malloc with non-integer element number");
        }

        static config::ConfigEntry<bool> NullableMallocs("analysis", "nullable-mallocs");
        if(NullableMallocs.get(true)) {
            return lhvp == ef.getNullPtr() || lhvp == ctx->getLocalPtr(elems);
        } else {
            return lhvp == ctx->getLocalPtr(elems);
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* MALLOCPREDICATE_H_ */
