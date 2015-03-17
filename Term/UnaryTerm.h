/*
 * UnaryTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef UNARYTERM_H_
#define UNARYTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/UnaryTerm.proto
import "Term/Term.proto";
import "Util/UnaryArithType.proto";

package borealis.proto;

message UnaryTerm {
    extend borealis.proto.Term {
        optional UnaryTerm ext = $COUNTER_TERM;
    }

    optional UnaryArithType opcode = 1;
    optional Term rhv = 2;
}

**/
class UnaryTerm: public borealis::Term {

    llvm::UnaryArithType opcode;

    UnaryTerm(Type::Ptr type, llvm::UnaryArithType opcode, Term::Ptr rhv);

public:

    MK_COMMON_TERM_IMPL(UnaryTerm);

    llvm::UnaryArithType getOpcode() const;
    Term::Ptr getRhv() const;

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> Term::Ptr {
        auto&& _rhv = tr->transform(getRhv());
        auto&& _type = getTermType(tr->FN.Type, _rhv);
        TERM_ON_CHANGED(
            getRhv() != _rhv,
            new Self( _type, opcode, _rhv )
        );
    }

    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

    static Type::Ptr getTermType(TypeFactory::Ptr, Term::Ptr rhv) {
        return rhv->getType();
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, UnaryTerm> {
    static Dynamic<Impl> doit(
            const UnaryTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {

        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        auto&& rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        switch (t->getOpcode()) {
        case llvm::UnaryArithType::BNOT: {
            auto&& rhvi = rhvz3.template to<Integer>();
            ASSERT(not rhvi.empty(), "Bit not: rhv is not an integer");
            return ~rhvi.getUnsafe();
        }
        case llvm::UnaryArithType::NEG: {
            auto&& rhvi = rhvz3.template to<Integer>();
            ASSERT(not rhvi.empty(), "Negate: rhv is not an integer");
            return -rhvi.getUnsafe();
        }
        case llvm::UnaryArithType::NOT: {
            auto&& rhvi = rhvz3.toBool();
            ASSERT(not rhvi.empty(), "Logic not: rhv is not a boolean");
            return not rhvi.getUnsafe();
        }
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* UNARYTERM_H_ */
