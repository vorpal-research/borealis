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

    unsigned int idx;

    ArgumentTerm(Type::Ptr type, unsigned int idx, const std::string& name) :
        Term(
            class_tag(*this),
            type,
            name
        ), idx(idx) {}

public:

    MK_COMMON_TERM_IMPL(ArgumentTerm);

    unsigned getIdx() const { return idx; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

    virtual bool equals(const Term* other) const override {
        if (const Self* that = llvm::dyn_cast_or_null<Self>(other)) {
            return Term::equals(other) &&
                    that->idx == idx;
        } else return false;
    }

    virtual size_t hashCode() const override {
        return util::hash::defaultHasher()(Term::hashCode(), idx);
    }

};

template<class Impl>
struct SMTImpl<Impl, ArgumentTerm> {
    static Dynamic<Impl> doit(
            const ArgumentTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        return ef.getVarByTypeAndName(t->getType(), t->getName());
    }
};

} /* namespace borealis */

#endif /* ARGUMENTTERM_H_ */
