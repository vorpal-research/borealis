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

    llvm::Function* F;

    ReturnValueTerm(llvm::Function* F, SlotTracker*) :
        Term(
            std::hash<llvm::Function*>()(F),
            "\\result_" + F->getName().str(),
            type_id(*this)
        ), F(F) {}

public:

    MK_COMMON_TERM_IMPL(ReturnValueTerm);

    llvm::Function* getFunction() const { return F; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(F->getFunctionType()->getReturnType());
    }

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
