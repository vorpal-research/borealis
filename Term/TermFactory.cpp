/*
 * TermFactory.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: ice-phoenix
 */

#include "Term/TermFactory.h"

#include "Util/macros.h"

namespace borealis {

template<class T, class Pool, class ...Args>
static Term::Ptr poolAlloc(Pool& pool, Args&&... args) {
    auto ptr = pool.newElement(std::forward<Args>(args)...);

    ptr->deleter().poolPtr = &pool;
    ptr->deleter().deleter = [](void* pool, const Term* vptr) {
        auto daPool = static_cast<Pool*>(pool);
        auto daObject = static_cast<const T*>(vptr);
        daPool->deleteElement(daObject);
    };
    return Term::Ptr(ptr);
};

template<class T, class ...Args>
static Term::Ptr make_pooled(Args &&... args) {
    using pool_t = MemoryPool<T, 128 * sizeof(T)>;
    static pool_t valuePool;

    return poolAlloc<T>(valuePool, std::forward<Args>(args)...);
};

template<class T, class ...Args>
static Term::Ptr make_cached(Args &&... args) {
    static std::unordered_map<std::tuple<std::decay_t<Args>...>, Term::Ptr> cache;
    auto&& cached = cache[std::tuple<std::decay_t<Args>...>{ args... }];
    return cached ? cached : (cached = Term::Ptr(new T(std::forward<Args>(args)...)));
}

template<class T, class ...Args>
static Term::Ptr make_cached_pooled(Args &&... args) {
    static std::unordered_map<std::tuple<std::decay_t<Args>...>, Term::Ptr> cache;
    auto&& cached = cache[std::tuple<std::decay_t<Args>...>{ args... }];
    return cached ? cached : (cached = make_pooled<T>(std::forward<Args>(args)...));
}

template<class T>
struct AllocationPoint {
    template<class ...Args>
    inline static Term::Ptr alloc(Args&&... args) { return Term::Ptr{ new T(std::forward<Args>(args)...) }; }
};

template<>
struct AllocationPoint<ValueTerm> {
    template<
        class ...Args,
        class = std::enable_if_t<not std::is_same<std::tuple<std::decay_t<Args>...>, std::tuple<ValueTerm>>::value>
    >
    inline static Term::Ptr alloc(Args&&... args) { return make_pooled<ValueTerm>(std::forward<Args>(args)...); }

    inline static Term::Ptr alloc(const ValueTerm& term) { return make_pooled<ValueTerm>(term); }
};

template<>
struct AllocationPoint<BinaryTerm> {
    template<class ...Args>
    inline static Term::Ptr alloc(Args&&... args) { return make_pooled<BinaryTerm>(std::forward<Args>(args)...); }
};

template<>
struct AllocationPoint<CmpTerm> {
    template<class ...Args>
    inline static Term::Ptr alloc(Args&&... args) { return make_pooled<CmpTerm>(std::forward<Args>(args)...); }
};

template<>
struct AllocationPoint<OpaqueIntConstantTerm> {
    template<class ...Args>
    inline static Term::Ptr alloc(Args&&... args) { return make_pooled<OpaqueIntConstantTerm>(std::forward<Args>(args)...); }
};

template<class T, class ...Args>
inline Term::Ptr make_new(Args &&... args) {
    return AllocationPoint<T>::alloc(std::forward<Args>(args)...);
};

TermFactory::TermFactory(SlotTracker* st, const llvm::DataLayout* DL, TypeFactory::Ptr TyF) :
    st(st), DL(DL), TyF(TyF) {}

Term::Ptr TermFactory::getArgumentTerm(const llvm::Argument* arg, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    return make_new<ArgumentTerm>(
        TyF->cast(arg->getType(), DL, sign),
        arg->getArgNo(),
        st->getLocalName(arg)
    );
}

Term::Ptr TermFactory::getArgumentTermExternal(size_t numero, const std::string& name, Type::Ptr type) {
    return make_new<ArgumentTerm>(
        type,
        numero,
        name
    );
}

Term::Ptr TermFactory::getArgumentCountTerm() {
    return make_new<ArgumentCountTerm>(TyF->getInteger());
}

Term::Ptr TermFactory::getVarArgumentTerm(size_t numero) {
    return make_new<VarArgumentTerm>(
        TyF->getUnknownType(),
        numero
    );
}

Term::Ptr TermFactory::getStringArgumentTerm(const llvm::Argument* arg, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    return make_new<ArgumentTerm>(
        TyF->cast(arg->getType(), DL, sign),
        arg->getArgNo(),
        st->getLocalName(arg),
        ArgumentKind::STRING
    );
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
            return getIntTerm(cInt->getValue(), sign);
        }
    } else if (auto cFP = dyn_cast<ConstantFP>(c)) {
        auto&& fp = cFP->getValueAPF();

        if (&fp.getSemantics() == &APFloat::IEEEsingle) {
            return getRealTerm(fp.convertToFloat());
        } else if (&fp.getSemantics() == &APFloat::IEEEdouble) {
            return getRealTerm(fp.convertToDouble());
        } else {
            auto refined = fp;
            bool success;
            refined.convert(APFloat::IEEEdouble, APFloat::rmNearestTiesToEven, &success);
            return getRealTerm(refined.convertToDouble());
        }

    } else if (auto undef = dyn_cast<UndefValue>(c)) {
        return getUndefTerm(undef);

    } else if (auto gv = dyn_cast<GlobalVariable>(c)) {
        // These guys should be processed separately by SeqDataPredicate
        // XXX: Keep in sync with FactoryNest
        return getGlobalValueTerm(gv);
    }

    return make_new<ConstTerm>(
        TyF->cast(c->getType(), DL),
        st->getLocalName(c)
    );
}

Term::Ptr TermFactory::getNullPtrTerm() {
    return make_new<OpaqueNullPtrTerm>(TyF->getUnknownType());
}

Term::Ptr TermFactory::getNullPtrTerm(const llvm::ConstantPointerNull* n) {
    return make_new<OpaqueNullPtrTerm>(TyF->cast(n->getType(), DL));
}

Term::Ptr TermFactory::getUndefTerm(const llvm::UndefValue* u) {
    return make_new<OpaqueUndefTerm>(TyF->cast(u->getType(), DL));
}

Term::Ptr TermFactory::getInvalidPtrTerm() {
    return make_new<OpaqueInvalidPtrTerm>(TyF->getUnknownType());
}

Term::Ptr TermFactory::getBooleanTerm(bool b) {
    return make_new<OpaqueBoolConstantTerm>(TyF->getBool(), b);
}

Term::Ptr TermFactory::getTrueTerm() {
    return getBooleanTerm(true);
}

Term::Ptr TermFactory::getFalseTerm() {
    return getBooleanTerm(false);
}

Term::Ptr TermFactory::getIntTerm(int64_t value, unsigned int size, llvm::Signedness sign) {
    return getIntTerm(value, TyF->getInteger(size, sign));
}

Term::Ptr TermFactory::getIntTerm(const std::string& representation, unsigned int size, llvm::Signedness sign) {
    if(size > sizeof(int64_t) * 8) {
        return make_new<OpaqueBigIntConstantTerm>(
            TyF->getInteger(size, sign), representation
        );
    }

    auto rep = util::fromString<uint64_t>(representation);
    ASSERTC(rep);

    return getIntTerm(rep.getUnsafe(), TyF->getInteger(size, sign));
}

Term::Ptr TermFactory::getIntTerm(const llvm::APInt& value, llvm::Signedness sign) {
    auto size = value.getBitWidth();
    if(size > sizeof(int64_t) * 8) {
        llvm::SmallString<30> ss;
        value.toStringUnsigned(ss);
        return make_new<OpaqueBigIntConstantTerm>(
            TyF->getInteger(size, sign), ss.str().str()
        );
    }

    return getIntTerm(value.getZExtValue(), TyF->getInteger(size, sign));// XXX: zext?
}

Term::Ptr TermFactory::getIntTerm(int64_t value, Type::Ptr type) {
    return make_new<OpaqueIntConstantTerm>(type, value);
}

Term::Ptr TermFactory::getRealTerm(double d) {
    return make_new<OpaqueFloatingConstantTerm>(
        TyF->getFloat(), d
    );
}

Term::Ptr TermFactory::getReturnValueTerm(const llvm::Function* F, llvm::Signedness sign) {
    ASSERT(st, "Missing SlotTracker");

    return make_new<ReturnValueTerm>(
        TyF->cast(F->getFunctionType()->getReturnType(), DL, sign),
        F->getName().str()
    );
}

Term::Ptr TermFactory::getReturnPtrTerm(const llvm::Function* F) {
    ASSERT(st, "Missing SlotTracker");

    return make_new<ReturnPtrTerm>(
        TyF->getPointer(TyF->cast(F->getFunctionType()->getReturnType(), DL)),
        F->getName().str()
    );
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



Term::Ptr TermFactory::getValueTerm(Type::Ptr type, const std::string& name, TermFactory::Globality globality) {
    return make_new<ValueTerm>(type, name, globality == Globality::Global);
}

Term::Ptr TermFactory::getFreeVarTerm(Type::Ptr type, const std::string& name) {
    return make_new<FreeVarTerm>(type, name);
}

Term::Ptr TermFactory::getGlobalValueTerm(const llvm::GlobalValue* gv, llvm::Signedness sign) {
    auto type = TyF->cast(gv->getType(), DL, sign);
    auto name = st->getLocalName(gv);
    return getValueTerm(type, name, Globality::Global);
}

Term::Ptr TermFactory::getLocalValueTerm(const llvm::Value* v, llvm::Signedness sign) {
    auto type = TyF->cast(v->getType(), DL, sign);
    auto name = st->getLocalName(v);
    return getValueTerm(type, name);
}

Term::Ptr TermFactory::getTernaryTerm(Term::Ptr cnd, Term::Ptr tru, Term::Ptr fls) {
    return make_new<TernaryTerm>(
        TernaryTerm::getTermType(TyF, cnd, tru, fls),
        cnd, tru, fls
    );
}

Term::Ptr TermFactory::getBinaryTerm(llvm::ArithType opc, Term::Ptr lhv, Term::Ptr rhv) {
    return make_new<BinaryTerm>(
        BinaryTerm::getTermType(TyF, lhv, rhv),
        opc, lhv, rhv
    );
}

Term::Ptr TermFactory::getUnaryTerm(llvm::UnaryArithType opc, Term::Ptr rhv) {
    return make_new<UnaryTerm>(
        UnaryTerm::getTermType(TyF, rhv),
        opc, rhv
    );
}

Term::Ptr TermFactory::getUnlogicLoadTerm(Term::Ptr rhv) {
    return getLoadTerm(getCastTerm(TyF->getPointer(TyF->getUnknownType()), false, rhv));
}

Term::Ptr TermFactory::getLoadTerm(Term::Ptr rhv) {
    return make_new<LoadTerm>(
        LoadTerm::getTermType(TyF, rhv),
        rhv
    );
}

Term::Ptr TermFactory::getReadPropertyTerm(Type::Ptr type, Term::Ptr propName, Term::Ptr rhv) {
    if(not llvm::isa<type::Pointer>(rhv->getType())) {
        rhv = getCastTerm(TyF->getPointer(TyF->getUnknownType()), false, rhv);
    }
    return make_new<ReadPropertyTerm>(type, propName, rhv);
}

Term::Ptr TermFactory::getGepTerm(Term::Ptr base, const std::vector<Term::Ptr>& shifts, bool isTriviallyInbounds) {

    auto&& tp = GepTerm::getGepChild(base->getType(), shifts);

    return make_new<GepTerm>(
        TyF->getPointer(tp),
        base,
        shifts,
        isTriviallyInbounds
    );
}

Term::Ptr TermFactory::getGepTerm(llvm::Value* base, const ValueVector& idxs, bool isTriviallyInbounds) {
    ASSERT(st, "Missing SlotTracker");

    using namespace llvm;
    using borealis::util::take;
    using borealis::util::view;

    auto&& type = GetElementPtrInst::getGEPReturnType(base, idxs);
    ASSERT(!!type, "getGepTerm: type after GEP is funked up");

    return make_new<GepTerm>(
        TyF->cast(type, DL),
        getValueTerm(base),
        util::viewContainer(idxs)
            .map([this](llvm::Value* idx) { return getValueTerm(idx); })
            .toVector(),
        isTriviallyInbounds
    );
}

Term::Ptr TermFactory::getCmpTerm(llvm::ConditionType opc, Term::Ptr lhv, Term::Ptr rhv) {
    return make_new<CmpTerm>(
        CmpTerm::getTermType(TyF, lhv, rhv),
        opc, lhv, rhv
    );
}

Term::Ptr TermFactory::getOpaqueVarTerm(const std::string& name) {
    return make_new<OpaqueVarTerm>(TyF->getUnknownType(), name);
}

Term::Ptr TermFactory::getOpaqueBuiltinTerm(const std::string& name) {
    return make_new<OpaqueBuiltinTerm>(TyF->getUnknownType(), name);
}

Term::Ptr TermFactory::getOpaqueNamedConstantTerm(const std::string& name) {
    return make_new<OpaqueNamedConstantTerm>(TyF->getUnknownType(), name);
}

Term::Ptr TermFactory::getOpaqueConstantTerm(int64_t v, size_t bitSize) {
    return make_new<OpaqueIntConstantTerm>(
        bitSize == 0 ? TyF->getUnknownType() : TyF->getInteger(bitSize),
        v
    );
}

Term::Ptr TermFactory::getOpaqueConstantTerm(double v) {
    return make_new<OpaqueFloatingConstantTerm>(TyF->getFloat(), v);
}

Term::Ptr TermFactory::getOpaqueConstantTerm(bool v) {
    return make_new<OpaqueBoolConstantTerm>(TyF->getBool(), v);
}

Term::Ptr TermFactory::getOpaqueConstantTerm(const char* v) {
    return make_new<OpaqueStringConstantTerm>(TyF->getUnknownType(), std::string{v});
}

Term::Ptr TermFactory::getOpaqueConstantTerm(const std::string& v) {
    return make_new<OpaqueStringConstantTerm>(
        TyF->getUnknownType(), v
    );
}

Term::Ptr TermFactory::getOpaqueIndexingTerm(Term::Ptr lhv, Term::Ptr rhv) {
    return make_new<OpaqueIndexingTerm>(
        TyF->getUnknownType(),
        lhv,
        rhv
    );
}

Term::Ptr TermFactory::getOpaqueMemberAccessTerm(Term::Ptr lhv, const std::string& property, bool indirect) {
    return make_new<OpaqueMemberAccessTerm>(
        TyF->getUnknownType(),
        lhv,
        property,
        indirect
    );
}

Term::Ptr TermFactory::getOpaqueCallTerm(Term::Ptr lhv, const std::vector<Term::Ptr>& rhv) {
    return make_new<OpaqueCallTerm>(
        TyF->getUnknownType(),
        lhv,
        rhv
    );
}

Term::Ptr TermFactory::getSignTerm(Term::Ptr rhv) {
    return make_new<SignTerm>(
        SignTerm::getTermType(TyF, rhv),
        rhv
    );
}

Term::Ptr TermFactory::getCastTerm(Type::Ptr type, bool signExtend, Term::Ptr rhv) {
    return make_new<CastTerm>(
        type,
        signExtend,
        rhv
    );
}

Term::Ptr TermFactory::getAxiomTerm(Term::Ptr lhv, Term::Ptr rhv) {
    return make_new<AxiomTerm>(
        AxiomTerm::getTermType(TyF, lhv, rhv),
        lhv, rhv
    );
}

Term::Ptr TermFactory::getBoundTerm(Term::Ptr rhv) {
    return make_new<BoundTerm>(
        TyF->getInteger(64, llvm::Signedness::Unsigned),
        rhv
    );
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

Term::Ptr TermFactory::setType(Type::Ptr type, Term::Ptr term) {
    if(false) {}
#define HANDLE_TERM(NAME, CLASS) \
    else if(auto&& ptr = llvm::dyn_cast<CLASS>(term)) { \
        auto copy = *ptr; \
        copy.type = type; \
        copy.update(); \
        return make_new<CLASS>(std::move(copy)); \
    }
#include "Term/Term.def"
    else BYE_BYE(Term::Ptr, "Unknown term type");
#undef HANDLE_TERM
}

} // namespace borealis

#include "Util/unmacros.h"
