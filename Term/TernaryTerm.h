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

    TernaryTerm(Type::Ptr type, Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls);

public:

    MK_COMMON_TERM_IMPL(TernaryTerm);

    Term::Ptr getCnd() const;
    Term::Ptr getTru() const;
    Term::Ptr getFls() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _cnd = tr->transform(getCnd());
        auto&& _tru = tr->transform(getTru());
        auto&& _fls = tr->transform(getFls());
        TERM_ON_CHANGED(
            getCnd() != _cnd || getTru() != _tru || getFls() != _fls,
            tr->FN.Term->getTernaryTerm(_cnd, _tru, _fls)
        );
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

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        Bool cndz3 = SMT<Impl>::doit(t->getCnd(), ef, ctx);
        auto&& truz3 = SMT<Impl>::doit(t->getTru(), ef, ctx);
        auto&& flsz3 = SMT<Impl>::doit(t->getFls(), ef, ctx);

        ASSERT(cndz3, "Ternary operation with non-Bool condition");

        return ef.if_(cndz3)
                 .then_(truz3)
                 .else_(flsz3);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* TERNARYTERM_H_ */
