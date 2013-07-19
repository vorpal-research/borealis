/*
 * ReturnValueTerm.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef RETURNVALUETERM_H_
#define RETURNVALUETERM_H_

#include <llvm/Function.h>

#include "Term/Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ReturnValueTerm: public borealis::Term {

    typedef ReturnValueTerm Self;

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    static bool classof(const Self*) {
        return true;
    }

    llvm::Function* getFunction() const { return F; }

    ReturnValueTerm(const Self&) = default;
    virtual ~ReturnValueTerm() {};

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(F->getFunctionType()->getReturnType());
    }

private:

    ReturnValueTerm(llvm::Function* F, SlotTracker*) :
        Term(
            std::hash<llvm::Function*>()(F),
            "\\result_" + F->getName().str(),
            type_id(*this)
        ), F(F) {}

    llvm::Function* F;

};

template<class Impl>
struct SMTImpl<Impl, ReturnValueTerm> {
    static Dynamic<Impl> doit(
            const ReturnValueTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getTermType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* RETURNVALUETERM_H_ */
