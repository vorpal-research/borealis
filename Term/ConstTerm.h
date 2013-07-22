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

namespace borealis {

class ConstTerm: public borealis::Term {

    llvm::Constant* c;

    ConstTerm(llvm::Constant* c, SlotTracker* st) :
        Term(std::hash<llvm::Constant*>()(c), st->getLocalName(c), type_id(*this)),
        c(c) {};

public:

    MK_COMMON_TERM_IMPL(ConstTerm);

    llvm::Constant* getConstant() const { return c; }

    template<class Sub>
    auto accept(Transformer<Sub>*) const -> const Self* {
        return new Self( *this );
    }

    virtual Type::Ptr getTermType() const override {
        return TypeFactory::getInstance().cast(c->getType());
    }

};

#include "Util/macros.h"
template<class Impl>
struct SMTImpl<Impl, ConstTerm> {
    static Dynamic<Impl> doit(
            const ConstTerm* t,
            ExprFactory<Impl>& ef,
            ExecutionContext<Impl>*) {
        using namespace llvm;

        USING_SMT_IMPL(Impl);

        // NB: you should keep this piece of code in sync with
        //     SlotTracker::getLocalName() method

        if (isa<ConstantPointerNull>(t->getConstant())) {
            return ef.getNullPtr();

        } else if (auto* cInt = dyn_cast<ConstantInt>(t->getConstant())) {
            if (cInt->getType()->getPrimitiveSizeInBits() == 1) {
                if (cInt->isOne()) return ef.getTrue();
                else if (cInt->isZero()) return ef.getFalse();
            } else {
                return ef.getIntConst(cInt->getValue().getZExtValue());
            }

        } else if (auto* cFP = dyn_cast<ConstantFP>(t->getConstant())) {
            auto& fp = cFP->getValueAPF();

            if (&fp.getSemantics() == &APFloat::IEEEsingle) {
                return ef.getRealConst(fp.convertToFloat());
            } else if (&fp.getSemantics() == &APFloat::IEEEdouble) {
                return ef.getRealConst(fp.convertToDouble());
            } else {
                BYE_BYE(Dynamic, "Unsupported semantics of APFloat");
            }

        } else if (dyn_cast<UndefValue>(t->getConstant())) {
            return ef.getVarByTypeAndName(t->getTermType(), t->getName(), true);

        }

        // FIXME: this is generally fucked up
        return ef.getVarByTypeAndName(t->getTermType(), t->getName());
    }
};
#include "Util/unmacros.h"

} /* namespace borealis */

#endif /* CONSTTERM_H_ */
