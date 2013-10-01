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
        optional OpaqueCallTerm ext = 37;
    }

    optional Term lhv = 1;
    repeated Term rhvs = 2;
}

**/
class OpaqueCallTerm: public borealis::Term {
    Term::Ptr lhv;
    std::vector<Term::Ptr> rhv;

    OpaqueCallTerm(Type::Ptr type, Term::Ptr lhv, const std::vector<Term::Ptr>& rhv):
        Term(
            class_tag(*this),
            type,
            lhv->getName() + util::toString(
                util::streams::delimited(
                    util::viewContainer(rhv).map([](Term::Ptr trm){ return trm->getName(); })
                )
            )
        ), lhv(lhv), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueCallTerm);

    Term::Ptr getLhv() const { return lhv; }
    const std::vector<Term::Ptr>& getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _lhv = tr->transform(lhv);
        auto _rhv = util::viewContainer(rhv).map([tr](Term::Ptr arg){
            return tr->transform(arg);
        }).toVector();
        auto _type = tr->FN.Type->getUnknownType();
        return new Self{ _type, _lhv, std::move(_rhv) };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    *that->lhv == *lhv &&
                    util::equal(
                        that->rhv,
                        rhv,
                        [](Term::Ptr thatptr, Term::Ptr thisptr){ return *thatptr == *thisptr; }
                    );
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), lhv, rhv);
    }
};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, OpaqueCallTerm> {
    static Dynamic<Impl> doit(
            const OpaqueCallTerm*,
            ExprFactory<Impl>&,
            ExecutionContext<Impl>*) {
        USING_SMT_IMPL(Impl);
        BYE_BYE(Dynamic, "Should not be called!");
    }
};
#include "Util/unmacros.h"

} // namespace borealis

#endif /* OPAQUECALLTERM_H_ */
