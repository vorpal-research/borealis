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

class TernaryTerm: public borealis::Term {

    typedef TernaryTerm Self;

    Term::Ptr cnd;
    Term::Ptr tru;
    Term::Ptr fls;

    TernaryTerm(Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls):
        Term(
            cnd->hashCode() ^ tru->hashCode() ^ fls->hashCode(),
            "(" + cnd->getName() + " ? " + tru->getName() + " : " + fls->getName() + ")",
            type_id(*this)
        ), cnd(cnd), tru(tru), fls(fls) {};

public:

    TernaryTerm(const Self&) = default;
    virtual ~TernaryTerm() {};

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        return new Self{ tr->transform(cnd), tr->transform(tru), tr->transform(fls) };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->cnd == *cnd &&
                    *that->tru == *tru &&
                    *that->fls == *fls;
        } else return false;
    }

    Term::Ptr getCnd() const { return cnd; }
    Term::Ptr getTru() const { return tru; }
    Term::Ptr getFls() const { return fls; }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    virtual Type::Ptr getTermType() const override {
        auto& tf = TypeFactory::getInstance();
        return tf.merge(tru->getTermType(), fls->getTermType());
    }

    friend class TermFactory;

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
