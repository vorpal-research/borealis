/*
 * WritePropertyPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef WRITEPROPERTYPREDICATE_H_
#define WRITEPROPERTYPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/WritePropertyPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message WritePropertyPredicate {
    extend borealis.proto.Predicate {
        optional WritePropertyPredicate ext = $COUNTER_PRED;
    }

    optional Term propName = 1;
    optional Term lhv = 2;
    optional Term rhv = 3;
}

**/
class WritePropertyPredicate: public borealis::Predicate {

    WritePropertyPredicate(
            Term::Ptr propName,
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(WritePropertyPredicate);

    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;
    Term::Ptr getPropertyName() const;

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _lhv = t->transform(getLhv());
        auto&& _rhv = t->transform(getRhv());
        auto&& _propName = t->transform(getPropertyName());
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv || getPropertyName() != _propName,
            new Self( _propName, _lhv, _rhv, _loc, _type )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, WritePropertyPredicate> {
    static Bool<Impl> doit(
            const WritePropertyPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        ASSERT(llvm::isa<OpaqueStringConstantTerm>(p->getPropertyName()),
               tfm::format("Property write with non-string property name: %s", p->getPropertyName()));
        auto* propName = llvm::cast<OpaqueStringConstantTerm>(p->getPropertyName());
        auto&& strPropName = propName->getValue();

        DynBV l = SMT<Impl>::doit(p->getLhv(), ef, ctx);
        ASSERT(l,
               tfm::format("Property write with a non-BV address: %s", p->getLhv()));
        auto&& lp = Pointer::forceCast(l);

        DynBV rbv = SMT<Impl>::doit(p->getRhv(), ef, ctx);
        ASSERT(rbv,
               tfm::format("Property write with a non-BV value: %s", p->getRhv()));

        ctx->writeProperty(strPropName, lp, rbv);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* WRITEPROPERTYPREDICATE_H_ */
