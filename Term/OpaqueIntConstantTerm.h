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

    typedef OpaqueIntConstantTerm Self;

    long long value;

    OpaqueIntConstantTerm(long long value):
        Term(
            static_cast<id_t>(value),
            borealis::util::toString(value),
            type_id(*this)
        ), value(value) {};

public:

    long long getValue() const { return value; }

    OpaqueIntConstantTerm(const Self&) = default;
    virtual ~OpaqueIntConstantTerm() {};

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

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getInteger();
    }

    friend class TermFactory;

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
