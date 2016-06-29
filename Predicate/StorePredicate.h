/*
 * StorePredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef STOREPREDICATE_H_
#define STOREPREDICATE_H_

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/StorePredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message StorePredicate {
    extend borealis.proto.Predicate {
        optional StorePredicate ext = $COUNTER_PRED;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class StorePredicate: public borealis::Predicate {

    StorePredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(StorePredicate);

    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _lhv = t->transform(getLhv());
        auto&& _rhv = t->transform(getRhv());
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv,
            new Self( _lhv, _rhv, _loc, _type )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, StorePredicate> {
    static Bool<Impl> doit(
            const StorePredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);
        size_t memspace = 0;
        if(auto&& ptr = llvm::dyn_cast<type::Pointer>(p->getLhv()->getType())) {
            memspace = ptr->getMemspace();
        }

        Pointer lp = SMT<Impl>::doit(p->getLhv(), ef, ctx);
        ASSERT(lp, "Store dealing with a non-pointer value");

        if(isa<type::Record>(p->getRhv()->getType())) {
            /// FIXME: for now, we do not do anything to store records
            /// record stores can only come from function calls returning records by value,
            /// which should be handled by contracts & stuff
            return ef.getTrue();
        }

        DynBV rbv = SMT<Impl>::doit(p->getRhv(), ef, ctx);
        ASSERT(rbv, "Store dealing with a non-BV value");

        ctx->writeExprToMemory(lp, rbv, memspace);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* STOREPREDICATE_H_ */
