/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include "Term/Term.h"
#include "Util/slottracker.h"

namespace borealis {

class ConstTerm: public borealis::Term {

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ConstTerm>();
    }

    static bool classof(const ConstTerm* /* t */) {
        return true;
    }

    llvm::Constant* getConstant() const {
        return constant;
    }

    ConstTerm(const ConstTerm&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().cast(constant->getType());
    }

private:

    ConstTerm(llvm::Constant* c, SlotTracker* st) :
        Term(std::hash<llvm::Constant*>()(c), llvm::valueType(*c), st->getLocalName(c), type_id(*this)),
        constant(c) {};

    llvm::Constant* constant;

};

} /* namespace borealis */

#endif /* CONSTTERM_H_ */
