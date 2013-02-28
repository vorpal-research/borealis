/*
 * ArgumentTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef ARGUMENTTERM_H_
#define ARGUMENTTERM_H_

#include <llvm/Argument.h>

#include "Term/Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ArgumentTerm: public borealis::Term {

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ArgumentTerm>();
    }

    static bool classof(const ArgumentTerm* /* t */) {
        return true;
    }

    llvm::Argument* getArgument() const {
        return a;
    }

    ArgumentTerm(const ArgumentTerm&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const {
        return z3ef.getExprForValue(*a, getName());
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().cast(a->getType());
    }

private:

    ArgumentTerm(llvm::Argument* a, SlotTracker* st) :
        Term(std::hash<llvm::Argument*>()(a), llvm::valueType(*a), st->getLocalName(a), type_id(*this)),
        a(a) {}

    llvm::Argument* a;

};

} /* namespace borealis */

#endif /* ARGUMENTTERM_H_ */
