//
// Created by abdullin on 2/14/19.
//

#include "AbstractFactory.hpp"
#include "DomainStorage.hpp"
#include "VariableFactory.hpp"
#include "Numerical/DoubleInterval.hpp"
#include "Numerical/IntervalDomain.hpp"
#include "Numerical/NumericalDomain.hpp"
#include "Memory/MemoryDomain.hpp"
#include "Memory/PointsToDomain.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

namespace impl_ {

template <typename IntervalT>
class IntervalDomainImpl : public IntervalDomain<IntervalT, const llvm::Value*> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = const llvm::Value*;
    using Self = IntervalDomainImpl<IntervalT>;
    using EnvT = typename IntervalDomain<IntervalT, const llvm::Value*>::EnvT;

protected:

    VariableFactory* vf_;

public:

    explicit IntervalDomainImpl(VariableFactory* vf) : IntervalDomain<IntervalT, const llvm::Value*>(), vf_(vf) {}
    IntervalDomainImpl(const IntervalDomainImpl&) = default;
    IntervalDomainImpl(IntervalDomainImpl&&) = default;
    IntervalDomainImpl& operator=(const IntervalDomainImpl&) = default;
    IntervalDomainImpl& operator=(IntervalDomainImpl&&) = default;
    ~IntervalDomainImpl() override = default;

    static Ptr top() { return std::make_shared<Self>(EnvT::top()); }
    static Ptr bottom() { return std::make_shared<Self>(EnvT::bottom()); }

    static bool classof (const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    Ptr get(Variable x) const override {
        // integer variable can't be global, so it's either constant or local
        if (auto&& constant = llvm::dyn_cast<llvm::Constant>(x)) {
            return vf_->get(constant);

        } else if (auto&& local = this->unwrapEnv()->get(x,
                [&]() -> AbstractDomain::Ptr { return vf_->bottom(x->getType()); },
                [&]() -> AbstractDomain::Ptr { return vf_->top(x->getType()); }
                )) {

            return local;

        }
        UNREACHABLE("Trying to create domain for unknown value type");
    }
};

template <typename MachineInt>
class PointsToDomainImpl : public PointsToDomain<MachineInt, const llvm::Value*> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = const llvm::Value*;
    using Self = PointsToDomainImpl<MachineInt>;
    using EnvT = typename PointsToDomain<MachineInt, const llvm::Value*>::EnvT;

protected:

    VariableFactory* vf_;

public:

    explicit PointsToDomainImpl(VariableFactory* vf) : PointsToDomain<MachineInt, const llvm::Value*>(), vf_(vf) {}
    PointsToDomainImpl(const PointsToDomainImpl&) = default;
    PointsToDomainImpl(PointsToDomainImpl&&) = default;
    PointsToDomainImpl& operator=(const PointsToDomainImpl&) = default;
    PointsToDomainImpl& operator=(PointsToDomainImpl&&) = default;
    ~PointsToDomainImpl() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static Ptr top() { return std::make_shared(EnvT::top()); }
    static Ptr bottom() { return std::make_shared(EnvT::bottom()); }

    static bool classof (const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr get(Variable x) const override {
        if (auto&& global = vf_->findGlobal(x)) {
            return global;

        } else if (auto&& constant = llvm::dyn_cast<llvm::Constant>(x)) {
            return vf_->get(constant);

        } else if (auto&& local = this->unwrapEnv()->get(x,
                [&]() -> AbstractDomain::Ptr { return PointerDomain<MachineInt>::bottom(); },
                [&]() -> AbstractDomain::Ptr { return PointerDomain<MachineInt>::top(); }
                )) {
            return local;

        }
        UNREACHABLE("Trying to create domain for unknown value type");
    }
};

} // namespace impl_

DomainStorage::NumericalDomainT* DomainStorage::unwrapFloat() const {
    auto* floats = llvm::dyn_cast<NumericalDomainT>(floats_.get());
    ASSERTC(floats);
    return floats;
}

DomainStorage::NumericalDomainT* DomainStorage::unwrapInt() const {
    auto* ints = llvm::dyn_cast<NumericalDomainT>(ints_.get());
    ASSERTC(ints);
    return ints;
}

DomainStorage::MemoryDomainT* DomainStorage::unwrapMemory() const {
    auto* mem = llvm::dyn_cast<MemoryDomainT>(memory_.get());
    ASSERTC(mem);
    return mem;
}

DomainStorage::DomainStorage(VariableFactory* vf) :
        ObjectLevelLogging("domain"),
        vf_(vf),
        ints_(std::make_shared<impl_::IntervalDomainImpl<DoubleInterval<SIntT, UIntT>>>(vf_)),
        floats_(std::make_shared<impl_::IntervalDomainImpl<Interval<Float>>>(vf_)),
        memory_(std::make_shared<impl_::PointsToDomainImpl<MachineIntT>>(vf_)) {}

DomainStorage::Ptr DomainStorage::clone() const {
    return std::make_shared<DomainStorage>(*this);
}

bool DomainStorage::equals(DomainStorage::Ptr other) const {
    return this->ints_->equals(other->ints_) && this->floats_->equals(other->floats_) && this->memory_->equals(other->memory_);
}

void DomainStorage::joinWith(DomainStorage::Ptr other) {
    this->ints_->joinWith(other->ints_);
    this->floats_->joinWith(other->floats_);
    this->memory_->joinWith(other->memory_);
}

DomainStorage::Ptr DomainStorage::join(DomainStorage::Ptr other) {
    auto&& result = std::make_shared<DomainStorage>(*this);
    result->joinWith(other);
    return result;
}

bool DomainStorage::empty() const {
    return ints_->isBottom() && floats_->isBottom() && memory_->isBottom();
}

AbstractDomain::Ptr DomainStorage::get(Variable x) const {
    auto&& type = vf_->cast(x->getType());

    if (llvm::isa<type::Integer>(type.get()))
        return unwrapInt()->get(x);
    else if (llvm::isa<type::Float>(type.get()))
        return unwrapFloat()->get(x);
    else if (llvm::isa<type::Pointer>(type.get()))
        return unwrapMemory()->get(x);
    else {
        warns() << "Variable '" << util::toString(*x) << "' of unknown type: " << TypeUtils::toString(*type.get()) << endl;
        return nullptr;
    }
}

void DomainStorage::assign(Variable x, Variable y) const {
    assign(x, get(y));
}

void DomainStorage::assign(Variable x, AbstractDomain::Ptr domain) const {
    auto&& type = vf_->cast(x->getType());

    if (llvm::isa<type::Integer>(type.get()))
        unwrapInt()->assign(x, domain);
    else if (llvm::isa<type::Float>(type.get()))
        unwrapFloat()->assign(x, domain);
    else if (llvm::isa<type::Pointer>(type.get()))
        unwrapMemory()->assign(x, domain);
    else {
        std::stringstream ss;
        ss << "Variable '" << util::toString(*x) << "' of unknown type: " << TypeUtils::toString(*type.get()) << std::endl;
        UNREACHABLE(ss.str())
    }
}

/// x = y op z
void DomainStorage::apply(llvm::BinaryOperator::BinaryOps op, Variable x, Variable y, Variable z) {
    typedef llvm::BinaryOperator::BinaryOps ops;
    auto aop = llvm::arithType(op);

    switch(op) {
        case ops::Add:
        case ops::Sub:
        case ops::Mul:
        case ops::Shl:
        case ops::LShr:
        case ops::AShr:
        case ops::And:
        case ops::Or:
        case ops::Xor:
        case ops::UDiv:
        case ops::URem:
        case ops::SDiv:
        case ops::SRem: {
            auto* ints = unwrapInt();
            ints->applyTo(aop, x, y, z);
            break;
        }

            // float operations
        case ops::FAdd:
        case ops::FSub:
        case ops::FMul:
        case ops::FDiv:
        case ops::FRem: {
            auto* fts = unwrapFloat();
            fts->applyTo(aop, x, y, z);
            break;
        }

        default: UNREACHABLE("Unreachable!");
    }
}

/// x = y op z
void DomainStorage::apply(llvm::CmpInst::Predicate op, Variable x, Variable y, Variable z) {
    typedef llvm::CmpInst::Predicate P;
    typedef llvm::ConditionType CT;

    auto cop = llvm::conditionType(op);
    auto* ints = unwrapInt();

    AbstractDomain::Ptr xd;
    if (P::FIRST_ICMP_PREDICATE <= op && op <= P::LAST_ICMP_PREDICATE) {
        if (y->getType()->isPointerTy() && z->getType()->isPointerTy()) {
            auto* memory = unwrapMemory();

            xd = memory->applyTo(cop, y, z);
        } else {
            xd = ints->applyTo(cop, y, z);
        }

    } else if (P::FIRST_FCMP_PREDICATE <= op && op <= P::LAST_FCMP_PREDICATE) {
        xd = ints->applyTo(cop, y, z);

    } else {
        UNREACHABLE("Unreachable!");
    }

    ints->assign(x, xd);
}

void DomainStorage::apply(CastOperator op, Variable x, Variable y) {
    auto&& yDom = get(y);
    auto&& targetType = vf_->cast(x->getType());

    auto&& xDom = vf_->af()->cast(op, targetType, yDom);
    assign(x, xDom);
}

/// x = *ptr
void DomainStorage::load(Variable x, Variable ptr) {
    auto&& xDom = unwrapMemory()->loadFrom(vf_->cast(x->getType()), ptr);
    assign(x, xDom);
}

/// *ptr = x
void DomainStorage::store(Variable ptr, Variable x) {
    unwrapMemory()->storeTo(ptr, get(x));
}

/// x = gep(ptr, shifts)
void DomainStorage::gep(Variable x, Variable ptr, const std::vector<Variable>& shifts) {
    std::vector<AbstractDomain::Ptr> normalizedShifts;
    for (auto&& it : shifts) {
        normalizedShifts.emplace_back(vf_->af()->machineIntInterval(get(it)));
    }
    unwrapMemory()->gepFrom(x, vf_->cast(x->getType()), ptr, normalizedShifts);
}

void DomainStorage::allocate(DomainStorage::Variable x, DomainStorage::Variable size) {
    auto&& sizeDomain = get(size);

    auto&& bounds = vf_->af()->unsignedBounds(sizeDomain);

    if (bounds.second.isInfinite()) {
        assign(x, vf_->top(x->getType()));
    } else {
        auto&& tf = vf_->tf();
        auto&& arrayType = tf->getArray(vf_->cast(x->getType()->getPointerElementType()), bounds.second.number());
        auto&& ptrType = tf->getPointer(arrayType, 0);
        assign(x, vf_->af()->allocate(ptrType));
    }
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

