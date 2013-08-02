/*
 * WritePropertyPredicate.h
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#ifndef WRITEPROPERTYPREDICATE_H_
#define WRITEPROPERTYPREDICATE_H_

#include "Protobuf/Gen/Predicate/WritePropertyPredicate.pb.h"

#include "Predicate/Predicate.h"

namespace borealis {

/** protobuf -> Predicate/WritePropertyPredicate.proto
import "Predicate/Predicate.proto";
import "Term/Term.proto";

package borealis.proto;

message WritePropertyPredicate {
    extend borealis.proto.Predicate {
        optional WritePropertyPredicate ext = 23;
    }

    optional Term propName = 1;
    optional Term lhv = 2;
    optional Term rhv = 3;
}

**/
class WritePropertyPredicate: public borealis::Predicate {

    Term::Ptr propName;
    Term::Ptr lhv;
    Term::Ptr rhv;

    WritePropertyPredicate(
            Term::Ptr propName,
            Term::Ptr lhv,
            Term::Ptr rhv,
            PredicateType type = PredicateType::STATE);

public:

    MK_COMMON_PREDICATE_IMPL(WritePropertyPredicate);

    Term::Ptr getPropertyName() const { return propName; }
    Term::Ptr getLhv() const { return lhv; }
    Term::Ptr getRhv() const { return rhv; }

    template<class SubClass>
    const Self* accept(Transformer<SubClass>* t) const {
        return new Self{
            t->transform(propName),
            t->transform(lhv),
            t->transform(rhv),
            type
        };
    }

    virtual bool equals(const Predicate* other) const override;
    virtual size_t hashCode() const override;

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, WritePropertyPredicate> {
    static Bool<Impl> doit(
            const WritePropertyPredicate* p,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;

        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        ASSERT(llvm::isa<ConstTerm>(p->getPropertyName()),
               "Property write with non-constant property name");
        auto* constPropName = llvm::cast<ConstTerm>(p->getPropertyName());
        auto strPropName = constPropName->getAsString();
        ASSERT(!strPropName.empty(),
               "Property write with unknown property name");

        auto l = SMT<Impl>::doit(p->getLhv(), ef, ctx).template to<Pointer>();
        ASSERT(!l.empty(), "Property write with a non-pointer value");
        auto lp = l.getUnsafe();

        auto r = SMT<Impl>::doit(p->getRhv(), ef, ctx);

        ctx->writeProperty(strPropName.getUnsafe(), lp, r);

        return ef.getTrue();
    }
};
#include "Util/unmacros.h"



template<class FN>
struct ConverterImpl<WritePropertyPredicate, proto::WritePropertyPredicate, FN> {

    typedef Converter<Term, proto::Term, FN> TermConverter;

    static proto::WritePropertyPredicate* toProtobuf(const WritePropertyPredicate* p) {
        auto res = util::uniq(new proto::WritePropertyPredicate());
        res->set_allocated_propname(
            TermConverter::toProtobuf(p->getPropertyName()).release()
        );
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
            const proto::WritePropertyPredicate& p) {
        auto propName = TermConverter::fromProtobuf(fn, p.propname());
        auto lhv = TermConverter::fromProtobuf(fn, p.lhv());
        auto rhv = TermConverter::fromProtobuf(fn, p.rhv());
        return Predicate::Ptr{ new WritePropertyPredicate(propName, lhv, rhv, type) };
    }
};

} /* namespace borealis */

#endif /* WRITEPROPERTYPREDICATE_H_ */
