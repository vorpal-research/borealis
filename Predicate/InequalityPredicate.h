/*
 * InequalityPredicate.h
 *
 *  Created on: Oct 26, 2012
 *      Author: ice-phoenix
 */

#ifndef INEQUALITYPREDICATE_H_
#define INEQUALITYPREDICATE_H_

#include "Protobuf/Gen/Predicate/InequalityPredicate.pb.h"

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/InequalityPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message InequalityPredicate {
    extend borealis.proto.Predicate {
        optional InequalityPredicate ext = 20;
    }

    optional Term lhv = 1;
    optional Term rhv = 2;
}

**/
class InequalityPredicate: public borealis::Predicate {

    Term::Ptr lhv;
    Term::Ptr rhv;

    InequalityPredicate(
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(InequalityPredicate);

    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(lhv),
            t->transform(rhv),
            type
        };
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

template<class Impl>
struct SMTImpl<Impl, InequalityPredicate> {
    static Bool<Impl> doit(
            const InequalityPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;
        return SMT<Impl>::doit(p->getLhv(), ef, ctx) != SMT<Impl>::doit(p->getRhv(), ef, ctx);
    }
};



template<class FN>
struct ConverterImpl<InequalityPredicate, proto::InequalityPredicate, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::InequalityPredicate* toProtobuf(const InequalityPredicate* p) {
        auto res = util::uniq(new proto::InequalityPredicate());
        res->set_allocated_lhv(
            TermConverter::toProtobuf(p->getLhv()).release()
        );
        res->set_allocated_rhv(
            TermConverter::toProtobuf(p->getRhv()).release()
        );
        return res.release();
    }

    static Predicate::Ptr fromProtobuf(
            FN fn,
            PredicateType type,
            const proto::InequalityPredicate& p) {
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{ new InequalityPredicate(lhv, rhv, type) };
    }
};

} /* namespace borealis */

#endif /* INEQUALITYPREDICATE_H_ */
