/*
 * VarArgumentTerm.h
 *
 *  Created on: Oct 30, 2015
 *      Author: belyaev
 */

#ifndef VARARGUMENTTERM_H_
#define VARARGUMENTTERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/VarArgumentTerm.proto
import "Term/Term.proto";

package borealis.proto;

message VarArgumentTerm {
    extend borealis.proto.Term {
        optional VarArgumentTerm ext = $COUNTER_TERM;
    }

    optional uint32 idx = 1;
}

**/
class VarArgumentTerm: public borealis::Term {

    unsigned int idx;

    VarArgumentTerm(Type::Ptr type, unsigned int idx);

public:

    MK_COMMON_TERM_IMPL(VarArgumentTerm);

    unsigned getIdx() const;

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

    virtual bool equals(const Term* other) const override;
    virtual size_t hashCode() const override;

};

template<class Impl>
struct SMTImpl<Impl, VarArgumentTerm> {
    static Dynamic<Impl> doit(
            const VarArgumentTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        TRACE_FUNC;
        USING_SMT_IMPL(Impl);

        // since it hasn't been materialized, it can be anything,
        // so let's make it a free var
        return ef.getPtrVar(tfm::format("$$bor.vararg.%d$$", t->getIdx()));
    }
};

} /* namespace borealis */

#endif /* VARARGUMENTTERM_H_ */
