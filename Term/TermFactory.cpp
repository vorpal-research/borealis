/*
 * TermFactory.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include "Term/TermFactory.h"

#include "Util/macros.h"

namespace borealis {

TermFactory::TermFactory(SlotTracker* st, const llvm::DataLayout* DL, TypeFactory::Ptr TyF) :
    st(st), DL(DL), TyF(TyF) {}

Term::Ptr TermFactory::getArgumentTerm(const llvm::Argument* arg, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    return Term::Ptr{
        new ArgumentTerm(
            TyF->cast(arg->getType(), DL, sign),
            arg->getArgNo(),
            st->getLocalName(arg)
        )
    };
}

Term::Ptr TermFactory::getArgumentTermExternal(size_t numero, const std::string& name, Type::Ptr type) {
    return Term::Ptr{
        new ArgumentTerm(
            type,
            numero,
            name
        )
    };
}

Term::Ptr TermFactory::getArgumentCountTerm() {
    return Term::Ptr{
        new ArgumentCountTerm(TyF->getInteger())
    };
}

Term::Ptr TermFactory::getVarArgumentTerm(size_t numero) {
    return Term::Ptr{
        new VarArgumentTerm(
            TyF->getUnknownType(),
            numero
        )
    };
}

Term::Ptr TermFactory::getStringArgumentTerm(const llvm::Argument* arg, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    return Term::Ptr{
        new ArgumentTerm(
            TyF->cast(arg->getType(), DL, sign),
            arg->getArgNo(),
            st->getLocalName(arg),
            ArgumentKind::STRING
        )
    };
}

Term::Ptr TermFactory::getConstTerm(const llvm::Constant* c, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    using namespace llvm;
    using borealis::util::tail;
    using borealis::util::view;

    if (auto cE = dyn_cast<ConstantExpr>(c)) {
        auto&& opcode = cE->getOpcode();

        if (opcode >= Instruction::CastOpsBegin && opcode <= Instruction::CastOpsEnd) {

            return getCastTerm(
                TyF->cast(cE->getType(), DL),
                opcode == Instruction::SExt,
                getValueTerm(cE->getOperand(0))
            );
        } else if (opcode == Instruction::GetElementPtr) {
            auto&& base = cE->getOperand(0);
            ValueVector idxs;
            idxs.reserve(cE->getNumOperands() - 1);
            for (auto&& i : tail(view(cE->op_begin(), cE->op_end()))) {
                idxs.push_back(i);
            }
            return getGepTerm(base, idxs, isTriviallyInboundsGEP(cE));
        }

    } else if (auto null = dyn_cast<ConstantPointerNull>(c)) {
        return getNullPtrTerm(null);

    } else if (auto cInt = dyn_cast<ConstantInt>(c)) {
        if (cInt->getType()->getPrimitiveSizeInBits() == 1) {
            if (cInt->isOne()) return getTrueTerm();
            else if (cInt->isZero()) return getFalseTerm();
        } else {
            return getIntTerm(cInt->getZExtValue(), cInt->getType()->getPrimitiveSizeInBits(), sign);
        }

    } else if (auto cFP = dyn_cast<ConstantFP>(c)) {
        auto&& fp = cFP->getValueAPF();

        if (&fp.getSemantics() == &APFloat::IEEEsingle) {
            return getRealTerm(fp.convertToFloat());
        } else if (&fp.getSemantics() == &APFloat::IEEEdouble) {
            return getRealTerm(fp.convertToDouble());
        } else {
            BYE_BYE(Term::Ptr, "Unsupported semantics of APFloat");
        }

    } else if (auto undef = dyn_cast<UndefValue>(c)) {
        return getUndefTerm(undef);

    } else if (auto gv = dyn_cast<GlobalVariable>(c)) {
        // These guys should be processed separately by SeqDataPredicate
        // XXX: Keep in sync with FactoryNest
        return Term::Ptr{
            new ValueTerm(
                TyF->cast(gv->getType(), DL),
                st->getLocalName(gv)
            )
        };

    }

    return Term::Ptr{
        new ConstTerm(
            TyF->cast(c->getType(), DL),
            st->getLocalName(c)
        )
    };
}

Term::Ptr TermFactory::getNullPtrTerm() {
    return Term::Ptr{
        new OpaqueNullPtrTerm(TyF->getUnknownType())
    };
}

Term::Ptr TermFactory::getNullPtrTerm(const llvm::ConstantPointerNull* n) {
    return Term::Ptr{
        new OpaqueNullPtrTerm(TyF->cast(n->getType(), DL))
    };
}

Term::Ptr TermFactory::getUndefTerm(const llvm::UndefValue* u) {
    return Term::Ptr{
        new OpaqueUndefTerm(TyF->cast(u->getType(), DL))
    };
}

Term::Ptr TermFactory::getInvalidPtrTerm() {
    return Term::Ptr{
        new OpaqueInvalidPtrTerm(TyF->getUnknownType())
    };
}

Term::Ptr TermFactory::getBooleanTerm(bool b) {
    return Term::Ptr{
        new OpaqueBoolConstantTerm(
            TyF->getBool(), b
        )
    };
}

Term::Ptr TermFactory::getTrueTerm() {
    return getBooleanTerm(true);
}

Term::Ptr TermFactory::getFalseTerm() {
    return getBooleanTerm(false);
}

Term::Ptr TermFactory::getIntTerm(long long value, unsigned int size, llvm::Signedness sign) {
    return Term::Ptr{
        new OpaqueIntConstantTerm(
            TyF->getInteger(size, sign), value
        )
    };
}

Term::Ptr TermFactory::getIntTerm(long long value, Type::Ptr type) {
    return Term::Ptr{
        new OpaqueIntConstantTerm(
            type, value
        )
    };
}

Term::Ptr TermFactory::getRealTerm(double d) {
    return Term::Ptr{
        new OpaqueFloatingConstantTerm(
            TyF->getFloat(), d
        )
    };
}

Term::Ptr TermFactory::getReturnValueTerm(const llvm::Function* F, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    return Term::Ptr{
        new ReturnValueTerm(
            TyF->cast(F->getFunctionType()->getReturnType(), DL, sign),
            F->getName().str()
        )
    };
}

Term::Ptr TermFactory::getReturnPtrTerm(const llvm::Function* F) {
    ASSERT(st, "Missing SlotTracker");

    return Term::Ptr{
        new ReturnPtrTerm(
            TyF->getPointer(TyF->cast(F->getFunctionType()->getReturnType(), DL)),
            F->getName().str()
        )
    };
}

Term::Ptr TermFactory::getValueTerm(const llvm::Value* v, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");
    using llvm::dyn_cast;

    if (auto gv = dyn_cast<llvm::GlobalValue>(v))
        return getGlobalValueTerm(gv, sign);
    else if (auto c = dyn_cast<llvm::Constant>(v))
        return getConstTerm(c, sign);
    else if (auto arg = dyn_cast<llvm::Argument>(v))
        return getArgumentTerm(arg, sign);
    else
        return getLocalValueTerm(v, sign);
}

Term::Ptr TermFactory::getValueTerm(Type::Ptr type, const std::string& name) {
    return Term::Ptr{ new ValueTerm(type, name) };
}

Term::Ptr TermFactory::getGlobalValueTerm(const llvm::GlobalValue* gv, llvm::Signedness sign) {
    return Term::Ptr{
        new ValueTerm(
            TyF->cast(gv->getType(), DL, sign),
            st->getLocalName(gv),
            /* global = */true
        )
    };
}

Term::Ptr TermFactory::getLocalValueTerm(const llvm::Value* v, llvm::Signedness sign) {
    return Term::Ptr{
        new ValueTerm(
            TyF->cast(v->getType(), DL, sign),
            st->getLocalName(v)
        )
    };
}

Term::Ptr TermFactory::getTernaryTerm(Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls) {
    return Term::Ptr{
        new TernaryTerm(
            TernaryTerm::getTermType(TyF, cnd, tru, fls),
            cnd, tru, fls
        )
    };
}

Term::Ptr TermFactory::getBinaryTerm(llvm::ArithType opc, Term::Ptr lhv, Term::Ptr rhv) {
    return Term::Ptr{
        new BinaryTerm(
            BinaryTerm::getTermType(TyF, lhv, rhv),
            opc, lhv, rhv
        )
    };
}

Term::Ptr TermFactory::getUnaryTerm(llvm::UnaryArithType opc, Term::Ptr rhv) {
    return Term::Ptr{
        new UnaryTerm(
            UnaryTerm::getTermType(TyF, rhv),
            opc, rhv
        )
    };
}

Term::Ptr TermFactory::getUnlogicLoadTerm(Term::Ptr rhv) {
    return getLoadTerm(getCastTerm(TyF->getPointer(TyF->getUnknownType()), false, rhv));
}

Term::Ptr TermFactory::getLoadTerm(Term::Ptr rhv) {
    return Term::Ptr{
        new LoadTerm(
            LoadTerm::getTermType(TyF, rhv),
            rhv
        )
    };
}

Term::Ptr TermFactory::getReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv) {
    return Term::Ptr{
        new ReadPropertyTerm(type, propName, rhv)
    };
}

Term::Ptr TermFactory::getGepTerm(Term::Ptr base, const std::vector<Term::Ptr>& shifts, bool isTriviallyInbounds) {

    auto&& tp = GepTerm::getGepChild(base->getType(), shifts);

    return Term::Ptr{
        new GepTerm{
            TyF->getPointer(tp),
            base,
            shifts,
            isTriviallyInbounds
        }
    };
}

Term::Ptr TermFactory::getGepTerm(llvm::Value* base, const ValueVector& idxs, bool isTriviallyInbounds) {
    ASSERT(st, "Missing SlotTracker");

    using namespace llvm;
    using borealis::util::take;
    using borealis::util::view;

    auto&& type = GetElementPtrInst::getGEPReturnType(base, idxs);
    ASSERT(!!type, "getGepTerm: type after GEP is funked up");

    return Term::Ptr{
        new GepTerm{
            TyF->cast(type, DL),
            getValueTerm(base),
            util::viewContainer(idxs)
                .map([this](llvm::Value* idx) { return getValueTerm(idx); })
                .toVector(),
            isTriviallyInbounds
        }
    };
}

Term::Ptr TermFactory::getCmpTerm(llvm::ConditionType opc, Term::Ptr lhv, Term::Ptr rhv) {
    return Term::Ptr{
        new CmpTerm(
            CmpTerm::getTermType(TyF, lhv, rhv),
            opc, lhv, rhv
        )
    };
}

Term::Ptr TermFactory::getOpaqueVarTerm(const std::string& name) {
    return Term::Ptr{
        new OpaqueVarTerm(TyF->getUnknownType(), name)
    };
}

Term::Ptr TermFactory::getOpaqueBuiltinTerm(const std::string& name) {
    return Term::Ptr{
        new OpaqueBuiltinTerm(TyF->getUnknownType(), name)
    };
}

Term::Ptr TermFactory::getOpaqueNamedConstantTerm(const std::string& name) {
    return Term::Ptr{
        new OpaqueNamedConstantTerm(TyF->getUnknownType(), name)
    };
}

Term::Ptr TermFactory::getOpaqueConstantTerm(long long v, size_t bitSize) {
    return Term::Ptr{
        new OpaqueIntConstantTerm(
            bitSize == 0 ? TyF->getUnknownType() : TyF->getInteger(bitSize),
            v
        )
    };
}

Term::Ptr TermFactory::getOpaqueConstantTerm(double v) {
    return Term::Ptr{
        new OpaqueFloatingConstantTerm(TyF->getFloat(), v)
    };
}

Term::Ptr TermFactory::getOpaqueConstantTerm(bool v) {
    return Term::Ptr{
        new OpaqueBoolConstantTerm(TyF->getBool(), v)
    };
}

Term::Ptr TermFactory::getOpaqueConstantTerm(const char* v) {
    return Term::Ptr{
        new OpaqueStringConstantTerm(
            TyF->getUnknownType(), std::string{v}
        )
    };
}

Term::Ptr TermFactory::getOpaqueConstantTerm(const std::string& v) {
    return Term::Ptr{
        new OpaqueStringConstantTerm(
            TyF->getUnknownType(), v
        )
    };
}

Term::Ptr TermFactory::getOpaqueIndexingTerm(Term::Ptr lhv, Term::Ptr rhv) {
    return Term::Ptr{
        new OpaqueIndexingTerm(
            TyF->getUnknownType(),
            lhv,
            rhv
        )
    };
}

Term::Ptr TermFactory::getOpaqueMemberAccessTerm(Term::Ptr lhv, const std::string& property, bool indirect) {
    return Term::Ptr{
        new OpaqueMemberAccessTerm(
            TyF->getUnknownType(),
            lhv,
            property,
            indirect
        )
    };
}

Term::Ptr TermFactory::getOpaqueCallTerm(Term::Ptr lhv, const std::vector<Term::Ptr>& rhv) {
    return Term::Ptr{
        new OpaqueCallTerm(
            TyF->getUnknownType(),
            lhv,
            rhv
        )
    };
}

Term::Ptr TermFactory::getSignTerm(Term::Ptr rhv) {
    return Term::Ptr{
        new SignTerm(
            SignTerm::getTermType(TyF, rhv),
            rhv
        )
    };
}

Term::Ptr TermFactory::getCastTerm(Type::Ptr type, bool signExtend, Term::Ptr rhv) {
    return Term::Ptr{
        new CastTerm(
            type,
            signExtend,
            rhv
        )
    };
}

Term::Ptr TermFactory::getAxiomTerm(Term::Ptr lhv, Term::Ptr rhv) {
    return Term::Ptr{
        new AxiomTerm(
            AxiomTerm::getTermType(TyF, lhv, rhv),
            lhv, rhv
        )
    };
}

Term::Ptr TermFactory::getBoundTerm(Term::Ptr rhv) {
    return Term::Ptr{
        new BoundTerm{
            TyF->getInteger(64, llvm::Signedness::Unsigned),
            rhv
        }
    };
}

TermFactory::Ptr TermFactory::get(SlotTracker* st, const llvm::DataLayout* DL, TypeFactory::Ptr TyF) {
    return TermFactory::Ptr{
        new TermFactory(st, DL, TyF)
    };
}

TermFactory::Ptr TermFactory::get(const llvm::DataLayout* DL, TypeFactory::Ptr TyF) {
    return TermFactory::Ptr{
        new TermFactory(nullptr, DL, TyF)
    };
}

} // namespace borealis

#include "Util/unmacros.h"
