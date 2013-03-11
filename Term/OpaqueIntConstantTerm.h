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

    typedef OpaqueIntConstantTerm self;

    long long value;

    OpaqueIntConstantTerm(long long value):
        Term(
            static_cast<id_t>(value),
            borealis::util::toString(value),
            type_id(*this)
        ), value(value) {};

public:

    long long getValue() const { return value; }

    OpaqueIntConstantTerm(const self&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const {
        return z3ef.getIntConst(value);
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    virtual bool equals(const Term* other) const {
        if(const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->value == value;
        } else return false;
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().getInteger();
    }

    friend class TermFactory;

};

} /* namespace borealis */

#endif /* OPAQUEINTCONSTANTTERM_H_ */
