/*
 * OpaqueBoolConstantTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBOOLCONSTANTTERM_H_
#define OPAQUEBOOLCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

class OpaqueBoolConstantTerm: public borealis::Term {

    bool value;

    OpaqueBoolConstantTerm(bool value):
        Term(
            static_cast<id_t>(value),
            value ? "true" : "false",
            type_id(*this)
        ), value(value) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueBoolConstantTerm);

    bool getValue() const { return value; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getBool();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueBoolConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueBoolConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getBoolConst(t->getValue());
    }
};

} /* namespace borealis */

#endif /* OPAQUEBOOLCONSTANTTERM_H_ */
