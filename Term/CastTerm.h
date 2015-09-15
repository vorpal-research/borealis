/*
 * CastTerm.cpp
 *
 *  Created on: Sep 7, 2015
 *      Author: ice-phoenix
 */

#ifndef CASTTERM_H_
#define CASTTERM_H_

#include "Term/Term.h"
#include "Util/cast.hpp"

namespace borealis {

/** protobuf -> Term/CastTerm.proto
import "Term/Term.proto";

package borealis.proto;

message CastTerm {
    extend borealis.proto.Term {
        optional CastTerm ext = $COUNTER_TERM;
    }

    optional Term rhv = 1;
    optional bool signExtend = 2;
}

**/
class CastTerm: public borealis::Term {

    CastTerm(Type::Ptr type, bool signExtend, Term::Ptr rhv);

    bool signExtend;

    void update();

public:

    MK_COMMON_TERM_IMPL(CastTerm);

    Term::Ptr getRhv() const;
    bool isSignExtend() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _rhv = tr->transform(getRhv());
        TERM_ON_CHANGED(
            getRhv() != _rhv,
            new Self( type, signExtend, _rhv )
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, CastTerm> {
    static Dynamic<Impl> doit(
            const CastTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        auto&& rhvbv = SMT<Impl>::doit(t->getRhv(), ef, ctx).template to<DynBV>();
        ASSERT(not rhvbv.empty(), "Cast for non BV");
        auto&& rhv = rhvbv.getUnsafe();

        auto&& lt = t->getType();
        auto&& rt = t->getRhv()->getType();

        if (auto m = util::match_tuple<type::Integer, type::Integer>::doit(lt, rt)) {
            if (m->get<0>()->getBitsize() > m->get<1>()->getBitsize()) {
                return t->isSignExtend()
                       ? rhv.growTo(m->get<0>()->getBitsize())
                       : rhv.zgrowTo(m->get<0>()->getBitsize());
            } else if (m->get<0>()->getBitsize() < m->get<1>()->getBitsize()) {
                return rhv.extract(m->get<0>()->getBitsize() - 1, 0);
            } else {
                return rhv;
            }
        }

        BYE_BYE(Dynamic, "Uncastable types: " + util::toString(*lt) + " and " + util::toString(*rt));
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CASTTERM_H_ */
