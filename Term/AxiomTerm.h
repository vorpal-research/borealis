/*
 * AxiomTerm.h
 *
 *  Created on: Aug 28, 2013
 *      Author: sam
 */

#ifndef AXIOMTERM_H_
#define AXIOMTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/AxiomTerm.proto
import "Term/Term.proto";

package borealis.proto;

message AxiomTerm {
    extend borealis.proto.Term {
        optional AxiomTerm ext = $COUNTER_TERM;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class AxiomTerm: public borealis::Term {

    AxiomTerm(Type::Ptr type, Term::Ptr lhv, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(AxiomTerm);

    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _lhv = tr->transform(getLhv());
        auto&& _rhv = tr->transform(getRhv());
        TERM_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv,
            tr->FN.Term->getAxiomTerm(_lhv, _rhv)
        );
    }

    static Type::Ptr getTermType(TypeFactory::Ptr, Term::Ptr lhv, Term::Ptr) {
        return lhv->getType();
    }

};

template<class Impl>
struct SMTImpl<Impl, AxiomTerm> {
    static Dynamic<Impl> doit(
            const AxiomTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        auto&& lhvsmt = SMT<Impl>::doit(t->getLhv(), ef, ctx);
        auto&& rhvsmt = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        return lhvsmt.withAxiom(rhvsmt);
    }
};

} /* namespace borealis */

#endif /* AXIOMTERM_H_ */
