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

    llvm::Argument* a;

    ArgumentTerm(llvm::Argument* a, SlotTracker* st) :
        Term(std::hash<llvm::Argument*>()(a), st->getLocalName(a), type_id(*this)),
        a(a) {}

public:

    MK_COMMON_TERM_IMPL(ArgumentTerm);

    llvm::Argument* getArgument() const { return a; }

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(a->getType());
    }

};

template<class Impl>
struct SMTImpl<Impl, ArgumentTerm> {
    static Dynamic<Impl> doit(
            const ArgumentTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getTermType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* ARGUMENTTERM_H_ */
