/*
 * BoundTerm.h
 *
 *  Created on: Oct 14, 2013
 *      Author: belyaev
 */

#ifndef BOUNDTERM_H_
#define BOUNDTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/BoundTerm.proto
import "Term/Term.proto";

package borealis.proto;

message BoundTerm {
    extend borealis.proto.Term {
        optional BoundTerm ext = $COUNTER_TERM;
    }

    optional Term rhv = 1;
}

**/
class BoundTerm: public borealis::Term {

    Term::Ptr rhv;

    BoundTerm(Type::Ptr type, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            "bound(" + rhv->getName() + ")"
        ), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(BoundTerm);

    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _rhv = tr->transform(rhv);
        auto _type = type;
        return new Self{ _type, _rhv };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), rhv);
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, BoundTerm> {
    static Dynamic<Impl> doit(
            const BoundTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        auto r = SMT<Impl>::doit(t->getRhv(), ef, ctx).template to<Pointer>();
        ASSERT(!r.empty(), "Bound read with non-pointer right side");
        auto rp = r.getUnsafe();

        return ctx->getBound(rp);
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* BOUNDTERM_H_ */
