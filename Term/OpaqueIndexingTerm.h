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

    OpaqueIndexingTerm(Type::Ptr type, Term::Ptr lhv, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(OpaqueIndexingTerm);

    Term::Ptr getLhv() const;
    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _lhv = tr->transform(getLhv());
        auto&& _rhv = tr->transform(getRhv());
        auto&& _type = tr->FN.Type->getUnknownType(); // FIXME: Can we do better?
        TERM_ON_CHANGED(
            getLhv() != _lhv || getRhv() != _rhv,
            new Self( _type, _lhv, _rhv )
        );
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
