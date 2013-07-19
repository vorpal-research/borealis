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

    typedef OpaqueBoolConstantTerm Self;

    bool value;

    OpaqueBoolConstantTerm(bool value):
        Term(
            static_cast<id_t>(value),
            value ? "true" : "false",
            type_id(*this)
        ), value(value) {};

public:

    bool getValue() const { return value; }

    OpaqueBoolConstantTerm(const Self&) = default;
    virtual ~OpaqueBoolConstantTerm() {};

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
        return TypeFactory::getInstance().getBool();
    }

    friend class TermFactory;

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
