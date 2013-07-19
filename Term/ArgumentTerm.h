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

    typedef ArgumentTerm Self;

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<Self>();
    }

    static bool classof(const Self*) {
        return true;
    }

    llvm::Argument* getArgument() const { return a; }

    ArgumentTerm(const Self&) = default;
    virtual ~ArgumentTerm() {};

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(a->getType());
    }

private:

    ArgumentTerm(llvm::Argument* a, SlotTracker* st) :
        Term(std::hash<llvm::Argument*>()(a), st->getLocalName(a), type_id(*this)),
        a(a) {}

    llvm::Argument* a;

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
