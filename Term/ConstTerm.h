/*
 * ConstTerm.h
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#ifndef CONSTTERM_H_
#define CONSTTERM_H_

#include <llvm/Constant.h>

#include "Term/Term.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

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
        return c;
    }

    ConstTerm(const ConstTerm&) = default;

    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));

    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const override {
        using namespace llvm;

        // NB: you should keep this piece of code in sync with
        //     SlotTracker::getLocalName() method

        if (isa<ConstantPointerNull>(c)) {
            return z3ef.getNullPtr();

        } else if (auto* cInt = dyn_cast<ConstantInt>(c)) {
            if (cInt->getType()->getPrimitiveSizeInBits() == 1) {
                if (cInt->isOne()) return z3ef.getTrue();
                else if (cInt->isZero()) return z3ef.getFalse();
            } else {
                return z3ef.getIntConst(cInt->getValue().getZExtValue());
            }

        } else if (auto* cFP = dyn_cast<ConstantFP>(c)) {
            auto& fp = cFP->getValueAPF();

            if (&fp.getSemantics() == &APFloat::IEEEsingle) {
                return z3ef.getRealConst(fp.convertToFloat());
            } else if (&fp.getSemantics() == &APFloat::IEEEdouble) {
                return z3ef.getRealConst(fp.convertToDouble());
            } else {
                BYE_BYE(Z3ExprFactory::Dynamic, "Unsupported semantics of APFloat");
            }

        } else if (dyn_cast<UndefValue>(c)) {
            return z3ef.getVarByTypeAndName(getTermType(), getName(), true);

        }

        // FIXME: this is generally fucked up
        return z3ef.getVarByTypeAndName(getTermType(), getName());
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(c->getType());
    }

private:

    ConstTerm(llvm::Constant* c, SlotTracker* st) :
        Term(std::hash<llvm::Constant*>()(c), st->getLocalName(c), type_id(*this)),
        c(c) {};

    llvm::Constant* c;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CONSTTERM_H_ */
