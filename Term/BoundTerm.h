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

    BoundTerm(Type::Ptr type, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(BoundTerm);

    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _rhv = tr->transform(getRhv());
        auto&& _type = type;
        TERM_ON_CHANGED(
            getRhv() != _rhv,
            new Self( _type, _rhv )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, BoundTerm> {
    static Dynamic<Impl> doit(
            const BoundTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        Pointer rp = SMT<Impl>::doit(t->getRhv(), ef, ctx);
        ASSERT(rp, "Bound read with non-pointer right side");

        return ctx->getBound(rp, ExprFactory::sizeForType(t->getType()));
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* BOUNDTERM_H_ */
