/*
 * TernaryTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef TERNARYTERM_H_
#define TERNARYTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/TernaryTerm.proto
import "Term/Term.proto";

package borealis.proto;

message TernaryTerm {
    extend borealis.proto.Term {
        optional TernaryTerm ext = $COUNTER_TERM;
    }

    optional Term cnd = 1;
    optional Term tru = 2;
    optional Term fls = 3;
}

**/
class TernaryTerm: public borealis::Term {

    Term::Ptr cnd;
    Term::Ptr tru;
    Term::Ptr fls;

    TernaryTerm(Type::Ptr type, Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls):
        Term(
            class_tag(*this),
            type,
            "(" + cnd->getName() + " ? " + tru->getName() + " : " + fls->getName() + ")"
        ), cnd(cnd), tru(tru), fls(fls) {};

public:

    MK_COMMON_TERM_IMPL(TernaryTerm);

    Term::Ptr getCnd() const { return cnd; }
    Term::Ptr getTru() const { return tru; }
    Term::Ptr getFls() const { return fls; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto _cnd = tr->transform(cnd);
        auto _tru = tr->transform(tru);
        auto _fls = tr->transform(fls);
        auto _type = getTermType(tr->FN.Type, _cnd, _tru, _fls);
        ON_CHANGED(
            cnd != _cnd || tru != _tru || fls != _fls,
            new Self( _type, _cnd, _tru, _fls )
        );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->cnd == *cnd &&
                    *that->tru == *tru &&
                    *that->fls == *fls;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), cnd, tru, fls);
    }

    static Type::Ptr getTermType(TypeFactory::Ptr TyF, Term::Ptr, Term::Ptr tru, Term::Ptr fls) {
        return TyF->merge(tru->getType(), fls->getType());
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, TernaryTerm> {
    static Dynamic<Impl> doit(
            const TernaryTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        auto cndz3 = SMT<Impl>::doit(t->getCnd(), ef, ctx);
        auto truz3 = SMT<Impl>::doit(t->getTru(), ef, ctx);
        auto flsz3 = SMT<Impl>::doit(t->getFls(), ef, ctx);

        ASSERT(cndz3.isBool(), "Ternary operation with non-Bool condition");

        auto cndb = cndz3.toBool().getUnsafe();

        return ef.if_(cndb)
                 .then_(truz3)
                 .else_(flsz3);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* TERNARYTERM_H_ */
