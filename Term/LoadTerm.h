/*
 * LoadTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef LOADTERM_H_
#define LOADTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/LoadTerm.proto
import "Term/Term.proto";

package borealis.proto;

message LoadTerm {
    extend borealis.proto.Term {
        optional LoadTerm ext = $COUNTER_TERM;
    }

    optional Term rhv = 1;
}

**/
class LoadTerm: public borealis::Term {

    LoadTerm(Type::Ptr type, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(LoadTerm);

    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _rhv = tr->transform(getRhv());
        TERM_ON_CHANGED(
            getRhv() != _rhv,
            tr->FN.Term->getLoadTerm(_rhv)
        );
    }

    static Type::Ptr getTermType(TypeFactory::Ptr TyF, Term::Ptr rhv) {
        auto&& type = rhv->getType();

        if (TypeUtils::isInvalid(type)) return type;

        if (auto* ptr = llvm::dyn_cast<type::Pointer>(type)) {
            return ptr->getPointed();
        } else {
            return TyF->getTypeError(
                "Load from a non-pointer: " + util::toString(*type)
            );
        }
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, LoadTerm> {
    static Dynamic<Impl> doit(
            const LoadTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        ASSERTC(ctx != nullptr);

        size_t memspace = 0;
        if(auto&& ptr = llvm::dyn_cast<type::Pointer>(t->getRhv()->getType())) {
            memspace = ptr->getMemspace();
        }

        Pointer rp = SMT<Impl>::doit(t->getRhv(), ef, ctx);
        ASSERT(rp, "Load with non-pointer right side");

        auto ret = ctx->readExprFromMemory(rp, ExprFactory::sizeForType(t->getType()), memspace);
        if(llvm::isa<type::Bool>(t->getType())) {
            return Bool::forceCast(ret);
        } else return ret;
    }
};
#include "Util/unmacros.h"

struct LoadTermExtractor {

    functional_hell::matchers::storage_t<Term::Ptr> unapply(Term::Ptr t) const {
        if (auto&& p = llvm::dyn_cast<LoadTerm>(t)) {
            return functional_hell::matchers::make_storage(p->getRhv());
        } else {
            return {};
        }
    }

};

static auto $LoadTerm = functional_hell::matchers::make_pattern(LoadTermExtractor());


} /* namespace borealis */

#endif /* LOADTERM_H_ */
