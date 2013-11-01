/*
 * ValueTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef VALUETERM_H_
#define VALUETERM_H_

#include "Term/Term.h"

namespace borealis {

/** protobuf -> Term/ValueTerm.proto
import "Term/Term.proto";

package borealis.proto;

message ValueTerm {
    extend borealis.proto.Term {
        optional ValueTerm ext = $COUNTER_TERM;
    }
}

**/
class ValueTerm: public borealis::Term {

    typedef std::unique_ptr<ValueTerm> SelfPtr;

    ValueTerm(Type::Ptr type, const std::string& name) :
        Term(
            class_tag(*this),
            type,
            name
        ) {};

public:

    MK_COMMON_TERM_IMPL(ValueTerm);

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> Term::Ptr {
        return this->shared_from_this();
    }

    Term::Ptr withNewName(const std::string& name) const {
        TERM_ON_CHANGED(
            this->name != name,
            new Self(this->type, name)
        );
    }

};

template<class Impl>
struct SMTImpl<Impl, ValueTerm> {
    static Dynamic<Impl> doit(
            const ValueTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* VALUETERM_H_ */
