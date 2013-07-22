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

    typedef std::unique_ptr<ValueTerm> SelfPtr;

    llvm::Value* v;

    ValueTerm(llvm::Value* v, SlotTracker* st) :
        Term(std::hash<llvm::Value*>()(v), st->getLocalName(v), type_id(*this)),
        v(v) {}

public:

    MK_COMMON_TERM_IMPL(ValueTerm);

    llvm::Value* getValue() const { return v; }

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(v->getType());
    }

    Term::Ptr withNewName(const std::string& name) const {
        auto res = SelfPtr{ new Self{ *this } };
        res->name = name;
        return Term::Ptr{ res.release() };
    }

};

template<class Impl>
struct SMTImpl<Impl, ValueTerm> {
    static Dynamic<Impl> doit(
            const ValueTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getTermType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* VALUETERM_H_ */
