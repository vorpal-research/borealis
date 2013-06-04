/*
 * TermFactory.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef TERMFACTORY_H_
#define TERMFACTORY_H_

#include <llvm/Value.h>

#include <memory>

#include "Codegen/llvm.h"

#include "Term/ArgumentTerm.h"
#include "Term/ConstTerm.h"
#include "Term/ReturnValueTerm.h"
#include "Term/Term.h"
#include "Term/ValueTerm.h"

#include "Term/BinaryTerm.h"
#include "Term/CmpTerm.h"
#include "Term/UnaryTerm.h"
#include "Term/TernaryTerm.h"

#include "Term/LoadTerm.h"

#include "Term/GepTerm.h"

#include "Term/OpaqueBoolConstantTerm.h"
#include "Term/OpaqueBuiltinTerm.h"
#include "Term/OpaqueFloatingConstantTerm.h"
#include "Term/OpaqueIntConstantTerm.h"
#include "Term/OpaqueVarTerm.h"

#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {

class TermFactory {

    typedef std::vector<llvm::Value*> ValueVector;

public:

    typedef std::unique_ptr<TermFactory> Ptr;

    Term::Ptr getArgumentTerm(llvm::Argument* a) {
        return Term::Ptr(new ArgumentTerm(a, slotTracker));
    }

    Term::Ptr getConstTerm(llvm::Constant* c) {
        using namespace llvm;
        using borealis::util::tail;
        using borealis::util::view;

        if (auto* cE = dyn_cast<ConstantExpr>(c)) {
            auto opcode = cE->getOpcode();

            if (opcode >= Instruction::CastOpsBegin && opcode <= Instruction::CastOpsEnd) {
                return getValueTerm(cE->getOperand(0));
            } else if (opcode == Instruction::GetElementPtr) {
                auto* base = cE->getOperand(0);
                ValueVector idxs;
                idxs.reserve(cE->getNumOperands() - 1);
                for (auto& i : tail(view(cE->op_begin(), cE->op_end()))) {
                    idxs.push_back(i);
                }
                return getGepTerm(base, idxs);
            }
        }

        return Term::Ptr(new ConstTerm(c, slotTracker));
    }

    Term::Ptr getNullPtrTerm() {
        return getConstTerm(llvm::getNullPointer());
    }

    Term::Ptr getBooleanTerm(bool b) {
        return Term::Ptr(new ConstTerm(llvm::getBoolConstant(b), slotTracker));
    }

    Term::Ptr getTrueTerm() {
        return getBooleanTerm(true);
    }

    Term::Ptr getFalseTerm() {
        return getBooleanTerm(false);
    }

    Term::Ptr getIntTerm(uint64_t i) {
        return Term::Ptr(new ConstTerm(llvm::getIntConstant(i), slotTracker));
    }

    Term::Ptr getReturnValueTerm(llvm::Function* F) {
        return Term::Ptr(new ReturnValueTerm(F, slotTracker));
    }

    Term::Ptr getValueTerm(llvm::Value* v) {
        using namespace llvm;

        if (auto* c = dyn_cast<Constant>(v))
            return getConstTerm(c);
        else if (auto* arg = dyn_cast<Argument>(v))
            return getArgumentTerm(arg);

        return Term::Ptr(new ValueTerm(v, slotTracker));
    }

    Term::Ptr getTernaryTerm(Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls) {
        return Term::Ptr(new TernaryTerm(cnd, tru, fls));
    }

    Term::Ptr getBinaryTerm(llvm::ArithType opc, Term::Ptr lhv, Term::Ptr rhv) {
        return Term::Ptr(new BinaryTerm(opc, lhv, rhv));
    }

    Term::Ptr getUnaryTerm(llvm::UnaryArithType opc, Term::Ptr rhv) {
        return Term::Ptr(new UnaryTerm(opc, rhv));
    }

    Term::Ptr getLoadTerm(Term::Ptr rhv) {
        return Term::Ptr(new LoadTerm(rhv));
    }

    Term::Ptr getGepTerm(llvm::Value* base, const ValueVector& idxs) {
        using namespace llvm;
        using borealis::util::tail;

        Term::Ptr v = getValueTerm(base);

        llvm::Type* type = base->getType();

        std::vector< std::pair<Term::Ptr, Term::Ptr> > shifts;
        shifts.reserve(idxs.size());

        for (auto* idx : idxs) {
            Term::Ptr by = getValueTerm(idx);
            Term::Ptr size = getIntTerm(getTypeSizeInElems(type));
            shifts.push_back({by, size});

            if (type->isPointerTy()) type = type->getPointerElementType();
            else if (type->isArrayTy()) type = type->getArrayElementType();
            else if (type->isStructTy()) {
                if (!isa<ConstantInt>(idx)) {
                    BYE_BYE(Term::Ptr, "Non-constant structure field shift");
                }
                auto fieldIdx = cast<ConstantInt>(idx)->getZExtValue();
                type = type->getStructElementType(fieldIdx);
            }
        }

        type = GetElementPtrInst::getGEPReturnType(base, idxs);

        return Term::Ptr(new GepTerm(v, shifts, type));
    }

    Term::Ptr getCmpTerm(llvm::ConditionType opc, Term::Ptr lhv, Term::Ptr rhv) {
        return Term::Ptr(new CmpTerm(opc, lhv, rhv));
    }

    Term::Ptr getOpaqueVarTerm(const std::string& name) {
        return Term::Ptr(new OpaqueVarTerm(name));
    }

    Term::Ptr getOpaqueBuiltinTerm(const std::string& name) {
        return Term::Ptr(new OpaqueBuiltinTerm(name));
    }

    Term::Ptr getOpaqueConstantTerm(long long v) {
        return Term::Ptr(new OpaqueIntConstantTerm(v));
    }

    Term::Ptr getOpaqueConstantTerm(double v) {
        return Term::Ptr(new OpaqueFloatingConstantTerm(v));
    }

    Term::Ptr getOpaqueConstantTerm(bool v) {
        return Term::Ptr(new OpaqueBoolConstantTerm(v));
    }

    static Ptr get(SlotTracker* slotTracker) {
        return Ptr(new TermFactory(slotTracker));
    }

private:

    SlotTracker* slotTracker;

    TermFactory(SlotTracker* slotTracker);

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* TERMFACTORY_H_ */
