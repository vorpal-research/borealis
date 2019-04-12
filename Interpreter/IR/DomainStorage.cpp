//
// Created by abdullin on 2/14/19.
//

#include <Config/config.h>
#include "Interpreter/Domain/AbstractFactory.hpp"
#include "DomainStorage.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"
#include "Interpreter/Domain/Numerical/DoubleInterval.hpp"
#include "Interpreter/Domain/Numerical/IntervalDomain.hpp"
#include "Interpreter/Domain/Numerical/Apron/OctagonDomain.hpp"
#include "Interpreter/Domain/Numerical/NumericalDomain.hpp"
#include "Interpreter/Domain/Memory/AggregateDomain.hpp"
#include "Interpreter/Domain/Memory/MemoryDomain.hpp"
#include "Interpreter/Domain/Memory/PointsToDomain.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ir {

namespace impl_ {

struct ValueEquals {
    bool operator()(const llvm::Value* lhv, const llvm::Value* rhv) const {
        return lhv == rhv;
    }
};

struct ValueHash {
    size_t operator()(const llvm::Value* lhv) const {
        return util::hash::defaultHasher()(lhv);
    }
};

template <typename IntervalT>
class IntervalDomainImpl : public IntervalDomain<IntervalT, const llvm::Value*, ValueHash, ValueEquals> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = const llvm::Value*;
    using Self = IntervalDomainImpl<IntervalT>;
    using EnvT = typename IntervalDomain<IntervalT, const llvm::Value*, ValueHash, ValueEquals>::EnvT;

protected:

    VariableFactory* vf_;

public:

    explicit IntervalDomainImpl(VariableFactory* vf) : IntervalDomain<IntervalT, const llvm::Value*, ValueHash, ValueEquals>(), vf_(vf) {}
    IntervalDomainImpl(const IntervalDomainImpl&) = default;
    IntervalDomainImpl(IntervalDomainImpl&&) = default;
    IntervalDomainImpl& operator=(const IntervalDomainImpl&) = default;
    IntervalDomainImpl& operator=(IntervalDomainImpl&&) = default;
    ~IntervalDomainImpl() override = default;

    static bool classof(const Self*) {
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
        return nullptr;
    }

    void addConstraint(llvm::ConditionType op, Variable x, Variable y) override {
        auto&& lhv = this->get(x);
        auto&& rhv = this->get(y);

        switch (op) {
            case llvm::ConditionType::EQ: {
                this->assign(x, lhv->splitByEq(rhv).true_);
                this->assign(y, rhv->splitByEq(lhv).true_);
                break;
            }
            case llvm::ConditionType::NEQ: {
                this->assign(x, lhv->splitByEq(rhv).false_);
                this->assign(y, rhv->splitByEq(lhv).false_);
                break;
            }
            case llvm::ConditionType::GT:
            case llvm::ConditionType::GE:
            case llvm::ConditionType::UGT:
            case llvm::ConditionType::UGE: {
                this->assign(x, lhv->splitByLess(rhv).false_);
                this->assign(y, rhv->splitByLess(lhv).true_);
                break;
            }
            case llvm::ConditionType::LT:
            case llvm::ConditionType::LE:
            case llvm::ConditionType::ULT:
            case llvm::ConditionType::ULE: {
                this->assign(x, lhv->splitByLess(rhv).true_);
                this->assign(y, rhv->splitByLess(lhv).false_);
                break;
            }
            case llvm::ConditionType::TRUE:break;
            case llvm::ConditionType::FALSE:break;
            case llvm::ConditionType::UNKNOWN:break;
            default:
                UNREACHABLE("Unknown operation")
        }
    }
};

template <typename N1, typename N2>
class OctagonDomainImpl : public OctagonDomain<N1, N2, const llvm::Value*, ValueHash, ValueEquals> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;
    using Variable = const llvm::Value*;
    using DOctagon = DoubleOctagon<N1, N2, const llvm::Value*, ValueHash, ValueEquals>;
    using OctagonMap = std::unordered_map<size_t, Ptr>;
    using Self = OctagonDomainImpl<N1, N2>;

protected:

    VariableFactory* vf_;

protected:

    size_t unwrapTypeSize(Variable x) const override {
        auto* integer = llvm::dyn_cast<type::Integer>(vf_->cast(x->getType()).get());
        ASSERTC(integer);

        return integer->getBitsize();
    }

    util::option<typename DOctagon::DNumber> unwrapDInterval(const typename DOctagon::DInterval* interval) {
        using DInt = typename DOctagon::DInterval;
        using DNum = typename DOctagon::DNumber;
        auto&& first = llvm::dyn_cast<typename DInt::Interval1>(interval->first().get());
        auto&& second = llvm::dyn_cast<typename DInt::Interval2>(interval->second().get());
        ASSERTC(first and second);

        // sometimes interval for llvm::Constant can be non-constant (for example, for constant expressions)
        if (first->isConstant() and second->isConstant())
            return util::just( DNum { first->asConstant(), second->asConstant() } );
        else
            return util::nothing();
    }

    util::option<typename DOctagon::DNumber> getConstant(Variable x) {
        if (auto&& constant = llvm::dyn_cast<llvm::Constant>(x)) {
            auto&& domain = vf_->get(constant);
            auto* domainRaw = llvm::dyn_cast<typename DOctagon::DInterval>(domain.get());
            ASSERTC(domainRaw);

            return unwrapDInterval(domainRaw);
        } else {
            return util::nothing();
        }
    }

public:

    explicit OctagonDomainImpl(VariableFactory* vf) : OctagonDomain<N1, N2, const llvm::Value*, ValueHash, ValueEquals>(), vf_(vf) {}
    OctagonDomainImpl(const OctagonDomainImpl&) = default;
    OctagonDomainImpl(OctagonDomainImpl&&) = default;
    OctagonDomainImpl& operator=(const OctagonDomainImpl& other) = default;
    OctagonDomainImpl& operator=(OctagonDomainImpl&&) = default;
    ~OctagonDomainImpl() override = default;

    static bool classof(const Self*) {
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

        } else {
            auto bitsize = unwrapTypeSize(x);
            auto* octagon = this->unwrapOctagon(bitsize);

            return octagon->get(x);
        }
    }

    bool equals(ConstPtr other) const override {
        // This is generaly fucked up, but for octagons it should be this way
        // otherwise interpreter goes to infinite loop
        return this->leq(other);
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        auto bitsize = this->unwrapTypeSize(x);
        auto* octagon = this->unwrapOctagon(bitsize);

        auto&& yConst = getConstant(y);
        auto&& zConst = getConstant(z);

        if (yConst && zConst) {
            octagon->applyTo(op, x, yConst.getUnsafe(), zConst.getUnsafe());
        } else if (yConst) {
            octagon->applyTo(op, x, yConst.getUnsafe(), z);
        } else if (zConst) {
            octagon->applyTo(op, x, y, zConst.getUnsafe());
        } else {
            octagon->applyTo(op, x, y, z);
        }
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        auto bitsize = this->unwrapTypeSize(x);
        auto* octagon = this->unwrapOctagon(bitsize);

        auto&& xConst = getConstant(x);
        auto&& yConst = getConstant(y);

        if (xConst && yConst) {
            return octagon->applyTo(op, xConst.getUnsafe(), yConst.getUnsafe());
        } else if (xConst) {
            return octagon->applyTo(op, xConst.getUnsafe(), y);
        } else if (yConst) {
            return octagon->applyTo(op, x, yConst.getUnsafe());
        } else {
            return octagon->applyTo(op, x, y);
        }
    }

    void addConstraint(llvm::ConditionType op, Variable x, Variable y) override {
        auto bitsize = this->unwrapTypeSize(x);
        auto* octagon = this->unwrapOctagon(bitsize);

        auto&& xConst = getConstant(x);
        auto&& yConst = getConstant(y);

        if (xConst && yConst) {
            return;
        } else if (xConst) {
            octagon->addConstraint(op, xConst.getUnsafe(), y);
        } else if (yConst) {
            octagon->addConstraint(op, x, yConst.getUnsafe());
        } else {
            octagon->addConstraint(op, x, y);
        }
    }

};

template <typename MachineInt>
class PointsToDomainImpl : public PointsToDomain<MachineInt, const llvm::Value*, ValueHash, ValueEquals> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = const llvm::Value*;
    using Self = PointsToDomainImpl<MachineInt>;
    using EnvT = typename PointsToDomain<MachineInt, const llvm::Value*, ValueHash, ValueEquals>::EnvT;

protected:

    VariableFactory* vf_;

public:

    explicit PointsToDomainImpl(VariableFactory* vf) : PointsToDomain<MachineInt, const llvm::Value*, ValueHash, ValueEquals>(), vf_(vf) {}
    PointsToDomainImpl(const PointsToDomainImpl&) = default;
    PointsToDomainImpl(PointsToDomainImpl&&) = default;
    PointsToDomainImpl& operator=(const PointsToDomainImpl&) = default;
    PointsToDomainImpl& operator=(PointsToDomainImpl&&) = default;
    ~PointsToDomainImpl() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
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
        return nullptr;
    }

    void addConstraint(llvm::ConditionType op, Variable x, Variable y) override {
        auto&& lhv = this->get(x);
        auto&& rhv = this->get(y);

        switch (op) {
            case llvm::ConditionType::EQ: {
                this->assign(x, lhv->splitByEq(rhv).true_);
                this->assign(y, rhv->splitByEq(lhv).true_);
                break;
            }
            case llvm::ConditionType::NEQ: {
                this->assign(x, lhv->splitByEq(rhv).false_);
                this->assign(y, rhv->splitByEq(lhv).false_);
                break;
            }
            // pointers support only equality/inequality check
            case llvm::ConditionType::GT:
            case llvm::ConditionType::GE:
            case llvm::ConditionType::UGT:
            case llvm::ConditionType::UGE:
            case llvm::ConditionType::LT:
            case llvm::ConditionType::LE:
            case llvm::ConditionType::ULT:
            case llvm::ConditionType::ULE:
            case llvm::ConditionType::TRUE:
            case llvm::ConditionType::FALSE:
            case llvm::ConditionType::UNKNOWN:break;
            default:
                UNREACHABLE("Unknown operation")
        }
    }
};

template <typename AggregateT>
class AggregateDomainImpl : public AggregateDomain<AggregateT, const llvm::Value*, ValueHash, ValueEquals> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = const llvm::Value*;
    using Self = AggregateDomainImpl<AggregateT>;
    using EnvT = typename AggregateDomain<AggregateT, const llvm::Value*, ValueHash, ValueEquals>::EnvT;

protected:

    VariableFactory* vf_;

public:

    explicit AggregateDomainImpl(VariableFactory* vf) : AggregateDomain<AggregateT, const llvm::Value*, ValueHash, ValueEquals>(), vf_(vf) {}
    AggregateDomainImpl(const AggregateDomainImpl&) = default;
    AggregateDomainImpl(AggregateDomainImpl&&) = default;
    AggregateDomainImpl& operator=(const AggregateDomainImpl&) = default;
    AggregateDomainImpl& operator=(AggregateDomainImpl&&) = default;
    ~AggregateDomainImpl() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
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
                [&]() -> AbstractDomain::Ptr { return vf_->bottom(x->getType()); },
                [&]() -> AbstractDomain::Ptr { return vf_->top(x->getType()); }
        )) {
            return local;

        }
        return nullptr;
    }
};

} // namespace impl_

static config::StringConfigEntry numericalDomain("absint", "numeric");

inline AbstractDomain::Ptr initNumericalDomain(VariableFactory* vf) {
    auto&& numericalDomainName = numericalDomain.get("interval");

    using SIntT = typename DomainStorage::SIntT;
    using UIntT = typename DomainStorage::UIntT;

    if (numericalDomainName == "interval") {
        return std::make_shared<impl_::IntervalDomainImpl<DoubleInterval<SIntT, UIntT>>>(vf);
    } else if (numericalDomainName == "octagon") {
        return std::make_shared<impl_::OctagonDomainImpl<SIntT, UIntT>>(vf);
    } else {
        UNREACHABLE("Unknown numerical domain name");
    }
}

DomainStorage::NumericalDomainT* DomainStorage::unwrapBool() const {
    auto* bools = llvm::dyn_cast<NumericalDomainT>(bools_.get());
    ASSERTC(bools);
    return bools;
}

DomainStorage::NumericalDomainT* DomainStorage::unwrapInt() const {
    auto* ints = llvm::dyn_cast<NumericalDomainT>(ints_.get());
    ASSERTC(ints);
    return ints;
}

DomainStorage::NumericalDomainT* DomainStorage::unwrapFloat() const {
    auto* floats = llvm::dyn_cast<NumericalDomainT>(floats_.get());
    ASSERTC(floats);
    return floats;
}

DomainStorage::MemoryDomainT* DomainStorage::unwrapMemory() const {
    auto* mem = llvm::dyn_cast<MemoryDomainT>(memory_.get());
    ASSERTC(mem);
    return mem;
}

DomainStorage::AggregateDomainT* DomainStorage::unwrapStruct() const {
    auto* structure = llvm::dyn_cast<AggregateDomainT>(structs_.get());
    ASSERTC(structure);
    return structure;
}

DomainStorage::DomainStorage(VariableFactory* vf) :
        ObjectLevelLogging("domain"),
        vf_(vf),
        bools_(std::make_shared<impl_::IntervalDomainImpl<Interval<UIntT>>>(vf_)),
        ints_(initNumericalDomain(vf_)),
        floats_(std::make_shared<impl_::IntervalDomainImpl<Interval<Float>>>(vf_)),
        memory_(std::make_shared<impl_::PointsToDomainImpl<MachineIntT>>(vf_)),
        structs_(std::make_shared<impl_::AggregateDomainImpl<StructDomain<MachineIntT>>>(vf_)) {}

DomainStorage::Ptr DomainStorage::clone() const {
    return std::make_shared<DomainStorage>(*this);
}

bool DomainStorage::equals(DomainStorage::Ptr other) const {
    return this->bools_->equals(other->bools_) &&
           this->ints_->equals(other->ints_) &&
           this->floats_->equals(other->floats_) &&
           this->memory_->equals(other->memory_) &&
           this->structs_->equals(other->structs_);
}

void DomainStorage::joinWith(DomainStorage::Ptr other) {
    this->bools_ = this->bools_->join(other->bools_);
    this->ints_ = this->ints_->join(other->ints_);
    this->floats_ = this->floats_->join(other->floats_);
    this->memory_ = this->memory_->join(other->memory_);
    this->structs_ = this->structs_->join(other->structs_);
}

DomainStorage::Ptr DomainStorage::join(DomainStorage::Ptr other) {
    auto&& result = std::make_shared<DomainStorage>(*this);
    result->joinWith(other);
    return result;
}

bool DomainStorage::empty() const {
    return bools_->isBottom() && ints_->isBottom() && floats_->isBottom() && memory_->isBottom() &&
           structs_->isBottom();
}

AbstractDomain::Ptr DomainStorage::get(Variable x) const {
    auto&& type = vf_->cast(x->getType());

    if (llvm::isa<type::Bool>(type.get()))
        return unwrapBool()->get(x);
    else if (auto* integer = llvm::dyn_cast<type::Integer>(type.get()))
        if (integer->getBitsize() == 1) return unwrapBool()->get(x);
        else return unwrapInt()->get(x);
    else if (llvm::isa<type::Float>(type.get()))
        return unwrapFloat()->get(x);
    else if (llvm::isa<type::Pointer>(type.get()))
        return unwrapMemory()->get(x);
    else if (llvm::isa<type::Record>(type.get()))
        return unwrapStruct()->get(x);
    else {
        return nullptr;
    }
}

void DomainStorage::assign(Variable x, Variable y) const {
    assign(x, get(y));
}

void DomainStorage::assign(Variable x, AbstractDomain::Ptr domain) const {
    auto&& type = vf_->cast(x->getType());

    if (llvm::isa<type::Bool>(type.get()))
        unwrapBool()->assign(x, domain);
    else if (auto* integer = llvm::dyn_cast<type::Integer>(type.get()))
        if (integer->getBitsize() == 1) unwrapBool()->assign(x, domain);
        else unwrapInt()->assign(x, domain);
    else if (llvm::isa<type::Float>(type.get()))
        unwrapFloat()->assign(x, domain);
    else if (llvm::isa<type::Pointer>(type.get()))
        unwrapMemory()->assign(x, domain);
    else if (llvm::isa<type::Record>(type.get()))
        unwrapStruct()->assign(x, domain);
    else {
        std::stringstream ss;
        ss << "Variable '" << util::toString(*x) << "' of unknown type: " << TypeUtils::toString(*type.get())
           << std::endl;
        UNREACHABLE(ss.str())
    }
}

/// x = y op z
void DomainStorage::apply(llvm::BinaryOperator::BinaryOps op, Variable x, Variable y, Variable z) {
    typedef llvm::BinaryOperator::BinaryOps ops;
    auto aop = llvm::arithType(op);

    switch (op) {
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
            if (y->getType()->getIntegerBitWidth() == 1) {
                auto* bools = unwrapBool();
                bools->applyTo(aop, x, y, z);

            } else {
                auto* ints = unwrapInt();
                ints->applyTo(aop, x, y, z);
            }
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

        default:
            UNREACHABLE("Unreachable!");
    }
}

/// x = y op z
void DomainStorage::apply(llvm::CmpInst::Predicate op, Variable x, Variable y, Variable z) {
    typedef llvm::CmpInst::Predicate P;
    typedef llvm::ConditionType CT;

    auto cop = llvm::conditionType(op);
    auto* bools = unwrapBool();

    AbstractDomain::Ptr xd;
    if (P::FIRST_ICMP_PREDICATE <= op && op <= P::LAST_ICMP_PREDICATE) {
        if (y->getType()->isPointerTy() && z->getType()->isPointerTy()) {
            auto* memory = unwrapMemory();

            xd = memory->applyTo(cop, y, z);
        } else {
            auto* ints = unwrapInt();

            xd = ints->applyTo(cop, y, z);
        }

    } else if (P::FIRST_FCMP_PREDICATE <= op && op <= P::LAST_FCMP_PREDICATE) {
        auto* floats = unwrapFloat();

        xd = floats->applyTo(cop, y, z);

    } else {
        UNREACHABLE("Unreachable!");
    }

    bools->assign(x, xd);
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

void DomainStorage::storeWithWidening(DomainStorage::Variable ptr, DomainStorage::Variable x) {
    auto&& ptrDom = unwrapMemory()->loadFrom(vf_->cast(x->getType()), ptr);
    auto&& xDom = get(x);
    unwrapMemory()->storeTo(ptr, ptrDom->widen(xDom));
}

/// x = gep(ptr, shifts)
void DomainStorage::gep(Variable x, Variable ptr, const std::vector<Variable>& shifts) {
    std::vector<AbstractDomain::Ptr> normalizedShifts;
    for (auto&& it : shifts) {
        normalizedShifts.emplace_back(vf_->af()->machineIntInterval(get(it)));
    }
    unwrapMemory()->gepFrom(x, vf_->cast(x->getType()), ptr, normalizedShifts);
}

/// x = extract(struct, index)
void DomainStorage::extract(Variable x, Variable structure, Variable index) {
    auto&& normalizedIndex = vf_->af()->machineIntInterval(get(index));
    auto&& xDom = unwrapStruct()->extractFrom(vf_->cast(x->getType()), structure, normalizedIndex);
    assign(x, xDom);
}

/// insert(struct, x, index)
void DomainStorage::insert(Variable structure, Variable x, Variable index) {
    auto&& normalizedIndex = vf_->af()->machineIntInterval(get(index));
    unwrapStruct()->insertTo(structure, get(x), normalizedIndex);
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

size_t DomainStorage::hashCode() const {
    return util::hash::defaultHasher()(bools_, ints_, floats_, memory_);
}

std::string DomainStorage::toString() const {
    std::stringstream ss;
    ss << "Bools:" << std::endl << bools_->toString() << std::endl;
    ss << "Ints:" << std::endl << ints_->toString() << std::endl;
    ss << "Floats:" << std::endl << floats_->toString() << std::endl;
    ss << "Memory:" << std::endl << memory_->toString() << std::endl;
    ss << "Structs:" << std::endl << structs_->toString() << std::endl;
    return ss.str();
}

std::pair<DomainStorage::Ptr, DomainStorage::Ptr> DomainStorage::split(Variable condition) const {
    auto&& true_ = std::make_shared<DomainStorage>(*this);
    auto&& false_ = std::make_shared<DomainStorage>(*this);

    auto&& condDomain = get(condition);
    if (condDomain->isTop() || condDomain->isBottom())
        return std::make_pair(true_, false_);

    auto* inst = llvm::dyn_cast<llvm::Instruction>(condition);
    if (not inst) return std::make_pair(true_, false_);
    else if (auto* icmp = llvm::dyn_cast<llvm::ICmpInst>(inst)) {
        auto cop = llvm::conditionType(icmp->getPredicate());
        auto ncop = llvm::makeNot(cop);

        auto* lhv = icmp->getOperand(0);
        auto* rhv = icmp->getOperand(1);

        if (icmp->getOperand(0)->getType()->isPointerTy()) {
            true_->unwrapMemory()->addConstraint(cop, lhv, rhv);
            false_->unwrapMemory()->addConstraint(ncop, lhv, rhv);
        } else {
            true_->unwrapInt()->addConstraint(cop, lhv, rhv);
            false_->unwrapInt()->addConstraint(ncop, lhv, rhv);
        }

    } else if (auto* fcmp = llvm::dyn_cast<llvm::FCmpInst>(inst)) {
        auto cop = llvm::conditionType(fcmp->getPredicate());
        auto ncop = llvm::makeNot(cop);

        auto* lhv = icmp->getOperand(0);
        auto* rhv = icmp->getOperand(1);

        true_->unwrapFloat()->addConstraint(cop, lhv, rhv);
        false_->unwrapFloat()->addConstraint(ncop, lhv, rhv);

    } else {
        warns() << "Binary operations in cmp are not supported" << endl;
    }
    return std::make_pair(true_, false_);
}

} // namespace ir
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

