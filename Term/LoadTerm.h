/*
 * LoadTerm.h
 *
 *  Created on: Jan 17, 2013
 *      Author: belyaev
 */

#ifndef LOADTERM_H_
#define LOADTERM_H_

#include "Term/Term.h"

namespace borealis {

class LoadTerm: public borealis::Term {
    typedef LoadTerm self;

    Term::Ptr rhv;

    LoadTerm(Term::Ptr rhv):
        Term(
                rhv->getId(),
                llvm::ValueType::PTR_VAR, // FIXME: infer the correct type?
                "*(" + rhv->getName() + ")",
                type_id(*this)
        ), rhv(rhv){};

public:

    LoadTerm(const LoadTerm&) = default;
    ~LoadTerm();

    template<class Sub>
    auto accept(Transformer<Sub>* tr) const -> const self* {
        return new LoadTerm(tr->transform(rhv));
    }

    virtual bool equals(const Term* other) const {
        if (const LoadTerm* that = llvm::dyn_cast<LoadTerm>(other)) {
            return  Term::equals(other) &&
                    *that->rhv == *rhv;
        } else return false;
    }

    Term::Ptr getRhv() const { return rhv; }

    static bool classof(const LoadTerm*) {
        return true;
    }

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<self>();
    }

    friend class TermFactory;
};

} /* namespace borealis */

#endif /* LOADTERM_H_ */
