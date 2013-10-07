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
    Term::Ptr rhv;

    UnaryTerm(Type::Ptr type, llvm::UnaryArithType opcode, Term::Ptr rhv):
        Term(
            class_tag(*this),
            type,
            llvm::unaryArithString(opcode) + "(" + rhv->getName() + ")"
        ), opcode(opcode), rhv(rhv) {};

public:

    MK_COMMON_TERM_IMPL(UnaryTerm);

    llvm::UnaryArithType getOpcode() const { return opcode; }
    Term::Ptr getRhv() const { return rhv; }

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const Self* {
        auto _rhv = tr->transform(rhv);
        auto _type = getTermType(tr->FN.Type, _rhv);
        return new Self{ _type, opcode, _rhv };
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->opcode == opcode &&
                    *that->rhv == *rhv;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), opcode, rhv);
    }

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

        USING_SMT_IMPL(Impl);

        auto rhvz3 = SMT<Impl>::doit(t->getRhv(), ef, ctx);

        switch(t->getOpcode()) {
        case llvm::UnaryArithType::BNOT: {
            auto rhvi = rhvz3.template to<Integer>();
            ASSERT(!rhvi.empty(), "Bit not: rhv is not an integer");
            return ~rhvi.getUnsafe();
        }
        case llvm::UnaryArithType::NEG: {
            auto rhvi = rhvz3.template to<Integer>();
            ASSERT(!rhvi.empty(), "Negate: rhv is not an integer");
            return -rhvi.getUnsafe();
        }
        case llvm::UnaryArithType::NOT: {
            auto rhvi = rhvz3.toBool();
            ASSERT(!rhvi.empty(), "Logic not: rhv is not a boolean");
            return !rhvi.getUnsafe();
        }
        }
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* UNARYTERM_H_ */
