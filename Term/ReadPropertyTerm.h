/*
 * ReadPropertyTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef READPROPERTYTERM_H_
#define READPROPERTYTERM_H_

#include "Term/OpaqueStringConstantTerm.h"
#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ReadPropertyTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ReadPropertyTerm {
    extend borealis.proto.Term {
        optional ReadPropertyTerm ext = $COUNTER_TERM;
    }

    optional Term propName = 1;
    optional Term rhv = 2;
}

**/
class ReadPropertyTerm: public borealis::Term {

    ReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(ReadPropertyTerm);

    Term::Ptr getRhv() const;
    Term::Ptr getPropertyName() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _rhv = tr->transform(getRhv());
        auto&& _propName = tr->transform(getPropertyName());
        TERM_ON_CHANGED(
            getRhv() != _rhv || getPropertyName() != _propName,
            tr->FN.Term->getReadPropertyTerm(type,  _propName, _rhv)
        );
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, ReadPropertyTerm> {
    static Dynamic<Impl> doit(
            const ReadPropertyTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        ASSERT(llvm::isa<OpaqueStringConstantTerm>(t->getPropertyName()),
               "Property read with non-string property name");
        auto* propName = llvm::cast<OpaqueStringConstantTerm>(t->getPropertyName());
        auto&& strPropName = propName->getValue();

        DynBV rp = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        ASSERT(rp, "Property read with non-bv right side");

        return ctx->readProperty(strPropName, Pointer::forceCast(rp), ExprFactory::sizeForType(t->getType()));
    }
};
#include "Util/unmacros.h"

struct ReadPropertyTermExtractor {

    functional_hell::matchers::storage_t<const std::string&, Term::Ptr> unapply(Term::Ptr t) const {
        if (auto&& p = llvm::dyn_cast<ReadPropertyTerm>(t)) {
            return functional_hell::matchers::make_storage(p->getPropertyName()->getName(), p->getRhv());
        } else {
            return {};
        }
    }

};

static auto $ReadPropertyTerm = functional_hell::matchers::make_pattern(ReadPropertyTermExtractor());


} /* namespace borealis */

#endif /* READPROPERTYTERM_H_ */
