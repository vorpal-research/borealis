/*
 * OpaqueIndexingTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEINDEXINGTERM_H_
#define OPAQUEINDEXINGTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueIndexingTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueIndexingTerm {
    extend borealis.proto.Term {
        optional OpaqueIndexingTerm ext = $COUNTER_TERM;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class OpaqueIndexingTerm: public borealis::Term {
    Term::Ptr lhv;
    Term::Ptr rhv;

    OpaqueIndexingTerm(Type::Ptr type, Term::Ptr lhv, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            lhv->getName() + "[" + rhv->getName() + "]"
        ), lhv(lhv), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueIndexingTerm);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto _lhv = tr->transform(lhv);
        auto _rhv = tr->transform(rhv);
        auto _type = tr->FN.Type->getUnknownType(); // FIXME: Can we do better?
        TERM_ON_CHANGED(
            lhv != _lhv || rhv != _rhv,
            new Self( _type, _lhv, _rhv )
        );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->lhv == *lhv &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), lhv, rhv);
    }
};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueIndexingTerm> {
    static Dynamic<Impl> doit(
            const OpaqueIndexingTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} // namespace borealis

#endif /* OPAQUEINDEXINGTERM_H_ */
