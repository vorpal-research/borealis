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

    ArgumentTerm(Type::Ptr type, unsigned int idx, const std::string& name, ArgumentKind kind = ArgumentKind::ANY) :
        Term(
            class_tag(*this),
            type,
            name
        ), idx(idx), kind(kind) {}

public:

    MK_COMMON_TERM_IMPL(ArgumentTerm);

    unsigned getIdx() const { return idx; }
    ArgumentKind getKind() const { return kind; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->idx == idx &&
                    that->kind == kind;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), idx, kind);
    }

};

template<class Impl>
struct SMTImpl<Impl, ArgumentTerm> {
    static Dynamic<Impl> doit(
            const ArgumentTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getType(), t->getName());
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
