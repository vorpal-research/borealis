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

    typedef ValueTerm Self;
    typedef std::unique_ptr<Self> SelfPtr;

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    static bool classof(const Self* /* t */) {
        return true;
    }

    llvm::Value* getValue() const {
        return v;
    }

    ValueTerm(const Self&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"


    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const override {
        return z3ef.getVarByTypeAndName(getTermType(), getName());
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(v->getType());
    }

    Term::Ptr withNewName(const std::string& name) const {
        auto res = SelfPtr{ util::heap_copy(this) };
        res->name = name;
        return Term::Ptr{ res.release() };
    }

private:

    ValueTerm(llvm::Value* v, SlotTracker* st) :
        Term(std::hash<llvm::Value*>()(v), st->getLocalName(v), type_id(*this)),
        v(v) {}

    llvm::Value* v;

};

} /* namespace borealis */

#endif /* VALUETERM_H_ */
