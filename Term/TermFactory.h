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
#include "Type/TypeFactory.h"
#include "Util/slottracker.h"

#include "Util/macros.h"

namespace borealis {

class TermFactory {

    typedef std::vector<llvm::Value*> ValueVector;

public:

    typedef std::shared_ptr<TermFactory> Ptr;

    Term::Ptr getArgumentTerm(llvm::Argument* arg) {
        return Term::Ptr{
            new ArgumentTerm(
                TyF->cast(arg->getType()),
                arg->getArgNo(),
                st->getLocalName(arg)
            )
        };
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

        } else if (auto* null = dyn_cast<ConstantPointerNull>(c)) {
            return getNullPtrTerm(null);

        } else if (auto* cInt = dyn_cast<ConstantInt>(c)) {
            if (cInt->getType()->getPrimitiveSizeInBits() == 1) {
                if (cInt->isOne()) return getTrueTerm();
                else if (cInt->isZero()) return getFalseTerm();
            } else {
                return getIntTerm(cInt->getValue().getZExtValue());
            }

        } else if (auto* cFP = dyn_cast<ConstantFP>(c)) {
            auto& fp = cFP->getValueAPF();

            if (&fp.getSemantics() == &APFloat::IEEEsingle) {
                return getRealTerm(fp.convertToFloat());
            } else if (&fp.getSemantics() == &APFloat::IEEEdouble) {
                return getRealTerm(fp.convertToDouble());
            } else {
                BYE_BYE(Term::Ptr, "Unsupported semantics of APFloat");
            }

        } else if (auto* undef = dyn_cast<UndefValue>(c)) {
            return getUndefTerm(undef);

        }

        return Term::Ptr{
            new ConstTerm(
                TyF->cast(c->getType()),
                st->getLocalName(c),
                getAsCompileTimeString(c)
            )
        };
    }

    Term::Ptr getNullPtrTerm() {
        return Term::Ptr{
            new OpaqueNullPtrTerm(TyF->getUnknown())
        };
    }

    Term::Ptr getNullPtrTerm(llvm::ConstantPointerNull* n) {
        return Term::Ptr{
            new OpaqueNullPtrTerm(TyF->cast(n->getType()))
        };
    }

    Term::Ptr getUndefTerm(llvm::UndefValue* u) {
        return Term::Ptr{
            new OpaqueUndefTerm(TyF->cast(u->getType()))
        };
    }

    Term::Ptr getBooleanTerm(bool b) {
        return Term::Ptr{
            new OpaqueBoolConstantTerm(
                TyF->getBool(), b
            )
        };
    }

    Term::Ptr getTrueTerm() {
        return getBooleanTerm(true);
    }

    Term::Ptr getFalseTerm() {
        return getBooleanTerm(false);
    }

    Term::Ptr getIntTerm(long long i) {
        return Term::Ptr{
            new OpaqueIntConstantTerm(
                TyF->getInteger(), i
            )
        };
    }

    Term::Ptr getRealTerm(double d) {
        return Term::Ptr{
            new OpaqueFloatingConstantTerm(
                TyF->getFloat(), d
            )
        };
    }

    Term::Ptr getReturnValueTerm(llvm::Function* F) {
        return Term::Ptr{
            new ReturnValueTerm(
                TyF->cast(F->getFunctionType()->getReturnType()),
                F->getName().str()
            )
        };
    }

    Term::Ptr getValueTerm(llvm::Value* v) {
        using namespace llvm;

        if (auto* c = dyn_cast<Constant>(v))
            return getConstTerm(c);
        else if (auto* arg = dyn_cast<Argument>(v))
            return getArgumentTerm(arg);

        return Term::Ptr{
            new ValueTerm(
                TyF->cast(v->getType()),
                st->getLocalName(v)
            )
        };
    }

    Term::Ptr getValueTerm(Type::Ptr type, const std::string& name) {
        return Term::Ptr{
            new ValueTerm(type, name)
        };
    }

    Term::Ptr getTernaryTerm(Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls) {
        return Term::Ptr{
            new TernaryTerm(
                TernaryTerm::getTermType(TyF, cnd, tru, fls),
                cnd, tru, fls
            )
        };
    }

    Term::Ptr getBinaryTerm(llvm::ArithType opc, Term::Ptr lhv, Term::Ptr rhv) {
        return Term::Ptr{
            new BinaryTerm(
                BinaryTerm::getTermType(TyF, lhv, rhv),
                opc, lhv, rhv
            )
        };
    }

    Term::Ptr getUnaryTerm(llvm::UnaryArithType opc, Term::Ptr rhv) {
        return Term::Ptr{
            new UnaryTerm(
                UnaryTerm::getTermType(TyF, rhv),
                opc, rhv
            )
        };
    }

    Term::Ptr getLoadTerm(Term::Ptr rhv) {
        return Term::Ptr{
            new LoadTerm(
                LoadTerm::getTermType(TyF, rhv),
                rhv
            )
        };
    }

    Term::Ptr getReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv) {
        return Term::Ptr{
            new ReadPropertyTerm(type, propName, rhv)
        };
    }

    Term::Ptr getGepTerm(llvm::Value* base, const ValueVector& idxs) {
        using namespace llvm;

        llvm::Type* baseType = base->getType();

        llvm::Type* type = baseType;
        ValueVector typeIdxs;
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

        return Term::Ptr{
            new GepTerm(
                TyF->cast(type),
                getValueTerm(base),
                shifts
            )
        };
    }

    Term::Ptr getCmpTerm(llvm::ConditionType opc, Term::Ptr lhv, Term::Ptr rhv) {
        return Term::Ptr{
            new CmpTerm(
                CmpTerm::getTermType(TyF, lhv, rhv),
                opc, lhv, rhv
            )
        };
    }

    Term::Ptr getOpaqueVarTerm(const std::string& name) {
        return Term::Ptr{
            new OpaqueVarTerm(TyF->getUnknown(), name)
        };
    }

    Term::Ptr getOpaqueBuiltinTerm(const std::string& name) {
        return Term::Ptr{
            new OpaqueBuiltinTerm(TyF->getUnknown(), name)
        };
    }

    Term::Ptr getOpaqueConstantTerm(long long v) {
        return Term::Ptr{
            new OpaqueIntConstantTerm(TyF->getInteger(), v)
        };
    }

    Term::Ptr getOpaqueConstantTerm(double v) {
        return Term::Ptr{
            new OpaqueFloatingConstantTerm(TyF->getFloat(), v)
        };
    }

    Term::Ptr getOpaqueConstantTerm(bool v) {
        return Term::Ptr{
            new OpaqueBoolConstantTerm(TyF->getBool(), v)
        };
    }

    static TermFactory::Ptr get(
            SlotTracker* st,
            TypeFactory::Ptr TyF) {
        return TermFactory::Ptr{
            new TermFactory(st, TyF)
        };
    }

private:

    SlotTracker* st;
    TypeFactory::Ptr TyF;

    TermFactory(SlotTracker* st, TypeFactory::Ptr TyF);

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* TERMFACTORY_H_ */
