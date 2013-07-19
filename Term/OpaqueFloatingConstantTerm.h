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

    typedef OpaqueFloatingConstantTerm Self;

    double value;

    OpaqueFloatingConstantTerm(double value):
        Term(
            std::hash<double>()(value),
            borealis::util::toString(value),
            type_id(*this)
        ), value(value) {};

public:

    double getValue() const { return value; }

    OpaqueFloatingConstantTerm(const Self&) = default;
    virtual ~OpaqueFloatingConstantTerm() {};

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    static bool classof(const Self*) {
        return true;
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

    friend class TermFactory;

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
