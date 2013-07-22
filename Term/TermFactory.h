/*
 * TermFactory.h
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#ifndef TERMFACTORY_H_
#define TERMFACTORY_H_

#include <memory>

#include "Codegen/llvm.h"
#include "Term/Term.def"
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
                auto* stripped = cE->stripPointerCasts();
                if (stripped != cE) return getValueTerm(stripped);

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

    Term::Ptr getReadPropertyTerm(Term::Ptr propName, Term::Ptr rhv, Type::Ptr type) {
        return Term::Ptr(new ReadPropertyTerm(propName, rhv, type));
    }

    Term::Ptr getGepTerm(llvm::Value* base, const ValueVector& idxs) {
        using namespace llvm;
        using borealis::util::tail;

        Term::Ptr v = getValueTerm(base);

        llvm::Type* baseType = base->getType();
        llvm::Type* type = baseType;

        std::vector< llvm::Value* > typeIdxs;
        typeIdxs.reserve(idxs.size());

        std::vector< std::pair<Term::Ptr, Term::Ptr> > shifts;
        shifts.reserve(idxs.size());

        for (auto* idx : idxs) {
            ASSERT(type, "Incorrect GEP type indices");

            Term::Ptr by = getValueTerm(idx);
            Term::Ptr size = getIntTerm(getTypeSizeInElems(type));
            shifts.push_back({by, size});

            typeIdxs.push_back(idx);
            type = GetElementPtrInst::getIndexedType(baseType, typeIdxs);
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

    static TermFactory::Ptr get(SlotTracker* slotTracker) {
        return TermFactory::Ptr(new TermFactory(slotTracker));
    }

private:

    SlotTracker* slotTracker;

    TermFactory(SlotTracker* slotTracker);

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* TERMFACTORY_H_ */
