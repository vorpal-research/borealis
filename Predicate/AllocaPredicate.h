/*
 * AllocaPredicate.h
 *
 *  Created on: Nov 28, 2012
 *      Author: belyaev
 */

#ifndef ALLOCAPREDICATE_H_
#define ALLOCAPREDICATE_H_

#include "Protobuf/Gen/Predicate/AllocaPredicate.pb.h"

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/AllocaPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message AllocaPredicate {
    extend borealis.proto.Predicate {
        optional AllocaPredicate ext = 16;
    }

    optional Term lhv = 1;
    optional Term numElements = 2;
}

**/
class AllocaPredicate: public borealis::Predicate {

    Term::Ptr lhv;
    Term::Ptr numElements;

    AllocaPredicate(
            Term::Ptr lhv,
            Term::Ptr numElements,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(AllocaPredicate);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getNumElems() const { return numElements; }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(lhv),
            t->transform(numElements),
            type
        };
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

        return lhvp == ctx->getDistinctPtr(elems);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* ALLOCAPREDICATE_H_ */
