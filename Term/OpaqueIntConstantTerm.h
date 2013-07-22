/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEINTCONSTANTTERM_H_
#define OPAQUEINTCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

class OpaqueIntConstantTerm: public borealis::Term {

    long long value;

    OpaqueIntConstantTerm(long long value):
        Term(
            static_cast<id_t>(value),
            borealis::util::toString(value),
            type_id(*this)
        ), value(value) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueIntConstantTerm);

    long long getValue() const { return value; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getInteger();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueIntConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueIntConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getIntConst(t->getValue());
    }
};

} /* namespace borealis */

#endif /* OPAQUEINTCONSTANTTERM_H_ */
