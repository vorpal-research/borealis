/*
 * AxiomTerm.h
 *
 *  Created on: Aug 28, 2013
 *      Author: sam
 */

#ifndef AXIOMTERM_H_
#define AXIOMTERM_H_

#include "Term/Term.h"

#include "Util/macros.h"

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

    Term::Ptr lhv;
    Term::Ptr rhv;

    AxiomTerm(Type::Ptr type, Term::Ptr lhv, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "(" + lhv->getName() + " with axiom " + rhv->getName() + ")"
        ), lhv(lhv), rhv(rhv) {
        ASSERT(llvm::isa<type::Bool>(rhv->getType()), "Attempt to add a non-Bool axiom term");
    };

public:

    MK_COMMON_TERM_IMPL(AxiomTerm);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _lhv = tr->transform(lhv);
        auto _rhv = tr->transform(rhv);
        auto _type = getTermType(tr->FN.Type, _lhv, _rhv);
        return new Self{ _type, _lhv, _rhv };
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

        USING_SMT_IMPL(Impl);

        auto lhvsmt = SMT<Impl>::doit(t->getLhv(), ef, ctx);
        auto rhvsmt = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        return lhvsmt.withAxiom(rhvsmt);
    }
};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* AXIOMTERM_H_ */
