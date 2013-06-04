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
        return constant;
    }

    ConstTerm(const ConstTerm&) = default;

    template<class Sub>
    auto accept(Transformer<Sub>*) QUICK_CONST_RETURN(util::heap_copy(this));

    virtual Z3ExprFactory::Dynamic toZ3(Z3ExprFactory& z3ef, ExecutionContext* = nullptr) const {
        using namespace llvm;

        if (isa<ConstantPointerNull>(constant)) {
            return z3ef.getNullPtr();
        } else if (auto* cInt = dyn_cast<ConstantInt>(constant)) {
            if (cInt->getType()->getPrimitiveSizeInBits() == 1) {
                if (cInt->isOne()) return z3ef.getTrue();
                else if (cInt->isZero()) return z3ef.getFalse();
            } else {
                return z3ef.getIntConst(cInt->getValue().getZExtValue());
            }
        } else if (auto* cFP = dyn_cast<ConstantFP>(constant)) {
            auto& fp = cFP->getValueAPF();

            if (&fp.getSemantics() == &APFloat::IEEEsingle) {
                return z3ef.getRealConst(fp.convertToFloat());
            } else if (&fp.getSemantics() == &APFloat::IEEEdouble) {
                return z3ef.getRealConst(fp.convertToDouble());
            } else {
                BYE_BYE(Z3ExprFactory::Dynamic, "Unsupported semantics of APFloat");
            }
        }

        // FIXME: this is generally fucked up
        return z3ef.getVarByTypeAndName(getTermType(), getName());
    }

    virtual Type::Ptr getTermType() const {
        return TypeFactory::getInstance().cast(constant->getType());
    }

private:

    ConstTerm(llvm::Constant* c, SlotTracker* st) :
        Term(std::hash<llvm::Constant*>()(c), st->getLocalName(c), type_id(*this)),
        constant(c) {};

    llvm::Constant* constant;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CONSTTERM_H_ */
