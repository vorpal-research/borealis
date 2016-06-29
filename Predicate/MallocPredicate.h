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
    optional Term origNumElements = 3;
}

**/
class MallocPredicate: public borealis::Predicate {

    MallocPredicate(
            Term::Ptr lhv,
            Term::Ptr numElems,
            Term::Ptr origNumElems,
            const Locus& loc,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(MallocPredicate);

    Term::Ptr getLhv() const;
    Term::Ptr getNumElems() const;
    Term::Ptr getOrigNumElems() const;

    template<class SubClass>
    Predicate::Ptr accept(Transformer<SubClass>* t) const {
        auto&& _lhv = t->transform(getLhv());
        auto&& _numElems = t->transform(getNumElems());
        auto&& _origNumElems = t->transform(getOrigNumElems());
        auto&& _loc = getLocation();
        auto&& _type = getType();
        PREDICATE_ON_CHANGED(
            getLhv() != _lhv || getNumElems() != _numElems || getOrigNumElems() != _origNumElems,
            new Self( _lhv, _numElems, _origNumElems, _loc, _type )
        );
    }

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
        size_t memspace = 0;
        if(auto&& ptr = llvm::dyn_cast<type::Pointer>(p->getLhv()->getType())) {
            memspace = ptr->getMemspace();
        }

        Pointer lhvp = SMT<Impl>::doit(p->getLhv(), ef, ctx);
        ASSERT(lhvp, "Malloc produces a non-pointer");

        auto&& elems = 1ULL;
        if (auto* cnst = llvm::dyn_cast<OpaqueIntConstantTerm>(p->getNumElems())) {
            elems = cnst->getValue();
        } else {
            BYE_BYE(Bool, "Encountered malloc with non-integer element number");
        }

        Integer origSize = SMT<Impl>::doit(p->getOrigNumElems(), ef, ctx);
        ASSERT(origSize, "Malloc with non-integer original size");

        static config::ConfigEntry<bool> NullableMallocs("analysis", "nullable-mallocs");
        if (NullableMallocs.get(true)) {
            return lhvp == ef.getNullPtr() || lhvp == ctx->getLocalPtr(memspace, elems, origSize);
        } else {
            return lhvp == ctx->getLocalPtr(memspace, elems, origSize);
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* MALLOCPREDICATE_H_ */
