/*
 * ArgumentTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef ARGUMENTTERM_H_
#define ARGUMENTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ArgumentKind.proto
package borealis.proto;

enum ArgumentKind {
    ANY    = 0;
    STRING = 1;
}
**/
enum class ArgumentKind {
    ANY    = 0,
    STRING = 1
};

/** protobuf -> Term/ArgumentTerm.proto
import "Term/Term.proto";
import "Term/ArgumentKind.proto";

package borealis.proto;

message ArgumentTerm {
    extend borealis.proto.Term {
        optional ArgumentTerm ext = $COUNTER_TERM;
    }

    optional uint32 idx = 1;
    optional ArgumentKind kind = 2;
}

**/
class ArgumentTerm: public borealis::Term {

    unsigned int idx;
    ArgumentKind kind;

    ArgumentTerm(Type::Ptr type, unsigned int idx, const std::string& name, ArgumentKind kind = ArgumentKind::ANY);

public:

    MK_COMMON_TERM_IMPL(ArgumentTerm);

    unsigned getIdx() const;
    ArgumentKind getKind() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

};

template<class Impl>
struct SMTImpl<Impl, ArgumentTerm> {
    static Dynamic<Impl> doit(
            const ArgumentTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>* ctx) {
        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        auto&& res = ef.getVarByTypeAndName(t->getType(), t->getName());

        if (not ctx) return res;
        if (not llvm::isa<type::Pointer>(t->getType())) return res;

        auto&& localMemoryBounds = ctx->getLocalMemoryBounds();
        return res.withAxiom(
            UComparable(res).ult(ef.getIntConst(localMemoryBounds.first)) ||
            UComparable(res).ugt(ef.getIntConst(localMemoryBounds.second))
        );
    }
};

} /* namespace borealis */

namespace std {
template<>
struct hash<borealis::ArgumentKind> : public borealis::util::enums::enum_hash<borealis::ArgumentKind> {};
template<>
struct hash<const borealis::ArgumentKind> : public borealis::util::enums::enum_hash<borealis::ArgumentKind> {};
}

#endif /* ARGUMENTTERM_H_ */
