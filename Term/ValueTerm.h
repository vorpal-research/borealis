/*
 * ValueTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef VALUETERM_H_
#define VALUETERM_H_

#include <llvm/Value.h>

#include "Term/Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ValueTerm: public borealis::Term {

public:

    friend class TermFactory;
    friend class GEPPredicate;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ValueTerm>();
    }

    static bool classof(const ValueTerm* /* t */) {
        return true;
    }

    llvm::Value* getValue() const {
        return v;
    }

    ValueTerm(const ValueTerm&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"


    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const {
        return z3ef.getExprForValue(*v, getName());
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().cast(v->getType());
    }


private:

    ValueTerm(llvm::Value* v, SlotTracker* st) :
        Term(std::hash<llvm::Value*>()(v), llvm::valueType(*v), st->getLocalName(v), type_id(*this)),
        v(v) {}

    llvm::Value* v;

};

} /* namespace borealis */

#endif /* VALUETERM_H_ */
