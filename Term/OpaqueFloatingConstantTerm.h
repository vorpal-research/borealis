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
    typedef OpaqueFloatingConstantTerm self;

    double value;

    OpaqueFloatingConstantTerm(double value):
        Term(
            std::hash<double>()(value),
            llvm::ValueType::REAL_CONST,
            borealis::util::toString(value),
            type_id(*this)
        ), value(value) {};

public:

    double getValue() { return value; }

    OpaqueFloatingConstantTerm(const self&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    virtual bool equals(const Term* other) const {
        if (const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->value - value == .0; // XXX: maybe bitwise is better?
        } else return false;
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().getFloat();
    }

    friend class TermFactory;
};

} /* namespace borealis */

#endif /* OPAQUEFLOATINGCONSTANTTERM_H_ */
