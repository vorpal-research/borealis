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

    typedef OpaqueBoolConstantTerm self;

    bool value;

    OpaqueBoolConstantTerm(bool value):
        Term(
            static_cast<id_t>(value),
            value ? "true" : "false",
            type_id(*this)
        ), value(value) {};

public:

    bool getValue() const { return value; }

    OpaqueBoolConstantTerm(const self&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const override {
        return z3ef.getBoolConst(value);
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().getBool();
    }

    friend class TermFactory;

};

} /* namespace borealis */

#endif /* OPAQUEBOOLCONSTANTTERM_H_ */
