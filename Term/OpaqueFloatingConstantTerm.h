/*
 * OpaqueConstantTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEFLOATINGCONSTANTTERM_H_
#define OPAQUEFLOATINGCONSTANTTERM_H_

#include "Term/Term.h"

namespace borealis {

class OpaqueFloatingConstantTerm: public borealis::Term {

    double value;

    OpaqueFloatingConstantTerm(double value):
        Term(
            std::hash<double>()(value),
            borealis::util::toString(value),
            type_id(*this)
        ), value(value) {};

public:

    MK_COMMON_TERM_IMPL(OpaqueFloatingConstantTerm);

    double getValue() const { return value; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    std::abs(that->value - value) < .01;
        } else return false;
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getFloat();
    }

};

template<class Impl>
struct SMTImpl<Impl, OpaqueFloatingConstantTerm> {
    static Dynamic<Impl> doit(
            const OpaqueFloatingConstantTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getRealConst(t->getValue());
    }
};

} /* namespace borealis */

#endif /* OPAQUEFLOATINGCONSTANTTERM_H_ */
