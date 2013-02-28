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
            llvm::ValueType::BOOL_CONST,
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

    // FIXME: toZ3???

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    static bool classof(const self*) {
        return true;
    }

    virtual bool equals(const Term* other) const {
        if (const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->value == value;
        } else return false;
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().getBool();
    }

    friend class TermFactory;

};

} /* namespace borealis */

#endif /* OPAQUEBOOLCONSTANTTERM_H_ */
