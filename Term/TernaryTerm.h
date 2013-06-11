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

    typedef TernaryTerm self;

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

    TernaryTerm(const TernaryTerm&) = default;
    ~TernaryTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new TernaryTerm(tr->transform(cnd), tr->transform(tru), tr->transform(fls));
    }

#include "Util/macros.h"
    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* ctx) const {
        auto cndz3 = cnd->toZ3(z3ef, ctx);
        auto truz3 = tru->toZ3(z3ef, ctx);
        auto flsz3 = fls->toZ3(z3ef, ctx);

        ASSERT(cndz3.isBool(), "Ternary operation with non-Bool condition");

        auto cndb = cndz3.toBool().getUnsafe();

        return z3ef.if_(cndb)
                   .then_(truz3)
                   .else_(flsz3);
    }
#include "Util/unmacros.h"

    virtual bool equals(const Term* other) const {
        if (const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    *that->cnd == *cnd &&
                    *that->tru == *tru &&
                    *that->fls == *fls;
        } else return false;
    }

    Term::Ptr getCnd() const { return cnd; }
    Term::Ptr getTru() const { return tru; }
    Term::Ptr getFls() const { return fls; }

    static bool classof(const self*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    virtual Type::Ptr getTermType() const {
        auto& tf = TypeFactory::getInstance();
        return tf.merge(tru->getTermType(), fls->getTermType());
    }

    friend class TermFactory;
};

} /* namespace borealis */

#endif /* TERNARYTERM_H_ */
