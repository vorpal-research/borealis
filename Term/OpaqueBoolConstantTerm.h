/*
 * OpaqueBoolConstantTerm.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef OPAQUEBOOLCONSTANTTERM_H_
#define OPAQUEBOOLCONSTANTTERM_H_

#include "Term.h"

namespace borealis {

class OpaqueBoolConstantTerm: public borealis::Term {
    typedef OpaqueBoolConstantTerm self;

    bool value;

    OpaqueBoolConstantTerm(bool value):
        Term(
            static_cast<id_t>(value),
            llvm::ValueType::UNKNOWN,
            borealis::util::toString(value),
            type_id(*this)
        ),
        value(value) {};
public:
    long long getValue() { return value; }

    OpaqueBoolConstantTerm(const self&) = default;

    friend class TermFactory;

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
        if(const self* that = llvm::dyn_cast<self>(other)) {
            return  Term::equals(other) &&
                    that->value == value;
        } else return false;
    }
};

} /* namespace borealis */
#endif /* OPAQUEBOOLCONSTANTTERM_H_ */
