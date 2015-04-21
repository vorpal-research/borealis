/*
 * OpaqueCallTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUECALLTERM_H_
#define OPAQUECALLTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/OpaqueCallTerm.proto
import "Term/Term.proto";

package borealis.proto;

message OpaqueCallTerm {
    extend borealis.proto.Term {
        optional OpaqueCallTerm ext = $COUNTER_TERM;
    }

    optional Term lhv = 1;
    repeated Term rhvs = 2;
}

**/
class OpaqueCallTerm: public borealis::Term {

    OpaqueCallTerm(Type::Ptr type, Term::Ptr lhv, const std::vector<Term::Ptr>& rhv);

public:

    MK_COMMON_TERM_IMPL(OpaqueCallTerm);

    Term::Ptr getLhv() const;
    auto getRhv() const -> decltype(util::viewContainer(subterms));

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _lhv = tr->transform(getLhv());
        auto&& _rhv = getRhv().map(
            [&](auto&& arg) { return tr->transform(arg); }
        );
        TERM_ON_CHANGED(
            getLhv() != _lhv || not util::equal(getRhv(), _rhv, ops::equals_to),
            new Self( tr->FN.Type->getUnknownType(), _lhv, _rhv.toVector() )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueCallTerm> {
    static Dynamic<Impl> doit(
            const OpaqueCallTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        BYE_BYE(Dynamic<Impl>, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} // namespace borealis

#endif /* OPAQUECALLTERM_H_ */
