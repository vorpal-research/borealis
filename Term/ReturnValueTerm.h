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

public:

    friend class TermFactory;

    static bool classof(const Term* t) {
        return t->getTermTypeId() == type_id<ReturnValueTerm>();
    }

    static bool classof(const ReturnValueTerm* /* t */) {
        return true;
    }

    llvm::Function* getFunction() const {
        return F;
    }

    ReturnValueTerm(const ReturnValueTerm&) = default;

#include "Util/macros.h"
    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));
#include "Util/unmacros.h"

private:

    ReturnValueTerm(llvm::Function* F, SlotTracker* /* st */) :
        Term(
                std::hash<llvm::Function*>()(F),
                llvm::type2type(*F->getFunctionType()->getReturnType()),
                "\\result",
                type_id(*this)
        ), F(F) {}

    llvm::Function* F;

};

} /* namespace borealis */

#endif /* RETURNVALUETERM_H_ */
