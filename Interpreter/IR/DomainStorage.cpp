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
};

template <typename N1, typename N2>
class DoubleOctagonImpl : public NumericalDomain<const llvm::Value*> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;
    using Variable = const llvm::Value*;
    using DOctagon = DoubleOctagon<N1, N2, const llvm::Value*, ValueHash, ValueEquals>;
    using OctagonMap = std::unordered_map<size_t, Ptr>;
    using Self = DoubleOctagonImpl<N1, N2>;

protected:

    VariableFactory* vf_;
    mutable OctagonMap octagons_;

private:

    Self* unwrap(Ptr other) const {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    const Self* unwrap(ConstPtr other) const {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    const type::Integer* unwrapType(Variable x) const {
        auto* integer = llvm::dyn_cast<type::Integer>(vf_->cast(x->getType()));
        ASSERTC(integer);

        return integer;
    }

    DOctagon* unwrapOctagon(size_t bitsize) const {
        auto&& opt = util::at(octagons_, bitsize);

        AbstractDomain::Ptr octagon;
        if (opt) {
            octagon = opt.getUnsafe();
        } else {
            octagon = std::make_shared<DOctagon>(util::Adapter<N1>::get(bitsize), util::Adapter<N2>::get(bitsize));
            octagons_[bitsize] = octagon;
        }

        auto* octagonRaw = llvm::dyn_cast<DOctagon>(octagon.get());
        ASSERTC(octagonRaw);
        return octagonRaw;
    }

public:

    explicit DoubleOctagonImpl(VariableFactory* vf) : NumericalDomain<const llvm::Value*>(class_tag(*this)), vf_(vf) {}
    DoubleOctagonImpl(const DoubleOctagonImpl&) = default;
    DoubleOctagonImpl(DoubleOctagonImpl&&) = default;
    DoubleOctagonImpl& operator=(DoubleOctagonImpl&&) = default;
    ~DoubleOctagonImpl() override = default;

    DoubleOctagonImpl& operator=(const DoubleOctagonImpl& other) {
        if (this == &other) {
            this->vf_ = other.vf_;
            this->octagons_ = other.octagons_;
        }
        return *this;
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    bool isTop() const override {
        bool top = true;
        for (auto&& it : octagons_)
            top &= it.second->isTop();
        return top;
    }

    bool isBottom() const override {
        bool top = true;
        for (auto&& it : octagons_)
            top &= it.second->isBottom();
        return top;
    }

    void setTop() override { UNREACHABLE("TODO"); }
    void setBottom() override { UNREACHABLE("TODO"); }
    bool leq(ConstPtr) const override { UNREACHABLE("TODO"); }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

        if (this->isBottom()) {
            return otherRaw->isBottom();
        } else if (otherRaw->isBottom()) {
            return false;
        } else {
            if (this->octagons_.size() != otherRaw->octagons_.size()) return false;

            for (auto&& it : this->octagons_) {
                auto&& otherIt = otherRaw->octagons_.find(it.first);
                if (otherIt == otherRaw->octagons_.end()) return false;
                else if (not it.second->equals((*otherIt).second)) return false;
            }
            return true;
        }
    }

    void joinWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->octagons_) {
                auto&& cur = octagons_.find(it.first);
                if (cur == octagons_.end()) octagons_[it.first] = it.second;
                else octagons_[it.first] = cur->second->join(it.second);
            }
            return;
        }
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (otherRaw->isBottom()) {
            this->setBottom();
        } else {
            for (auto&& it : otherRaw->octagons_) {
                auto&& cur = octagons_.find(it.first);
                if (cur == octagons_.end()) octagons_[it.first] = it.second;
                else octagons_[it.first] = cur->second->meet(it.second);
            }
            return;
        }
    }

    void widenWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->octagons_) {
                auto&& cur = octagons_.find(it.first);
                if (cur == octagons_.end()) octagons_[it.first] = it.second;
                else octagons_[it.first] = cur->second->widen(it.second);
            }
            return;
        }
    }

    Ptr join(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->joinWith(other);
        return result;
    }

    Ptr meet(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->meetWith(other);
        return result;
    }

    Ptr widen(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->widenWith(other);
        return result;
    }

    Ptr toInterval(Variable x) const override { return this->get(x); }

    Ptr get(Variable x) const override {
        // integer variable can't be global, so it's either constant or local
        if (auto&& constant = llvm::dyn_cast<llvm::Constant>(x)) {
            return vf_->get(constant);

        } else {
            auto* integer = unwrapType(x);
            auto* octagon = unwrapOctagon(integer->getBitsize());

            return octagon->get(x);
        }
    }

    void assign(Variable x, Variable y) override {
        auto* integer = unwrapType(x);
        auto* octagon = unwrapOctagon(integer->getBitsize());
        octagon->assign(x, y);
    }

    void assign(Variable x, Ptr i) override {
        auto* integer = unwrapType(x);
        auto* octagon = unwrapOctagon(integer->getBitsize());
        octagon->assign(x, i);
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        auto* integer = unwrapType(x);
        auto* octagon = unwrapOctagon(integer->getBitsize());
        octagon->applyTo(op, x, y, z);
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        auto* integer = unwrapType(x);
        auto* octagon = unwrapOctagon(integer->getBitsize());
        return octagon->applyTo(op, x, y);
    }

    size_t hashCode() const override {
        return class_tag(*this);
    }

    std::string toString() const override {
        std::stringstream ss;
        for (auto&& it : octagons_) {
            ss << "Bitsize " << it.first << std::endl;
            ss << it.second->toString();
        }
        return ss.str();
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
        return std::make_shared<impl_::DoubleOctagonImpl<SIntT, UIntT>>(vf);
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

    auto&& splitted = std::move(handleInst(inst));
    for (auto&& it : splitted) {
        true_->assign(it.first, it.second.true_);
        false_->assign(it.first, it.second.false_);
    }
    return std::make_pair(true_, false_);
}

std::unordered_map<DomainStorage::Variable, Split> DomainStorage::handleInst(const llvm::Instruction* target) const {
    if (auto* icmp = llvm::dyn_cast<llvm::ICmpInst>(target))
        return std::move(handleIcmp(icmp));
    else if (auto* fcmp = llvm::dyn_cast<llvm::FCmpInst>(target))
        return std::move(handleFcmp(fcmp));
    else if (auto* binary = llvm::dyn_cast<llvm::BinaryOperator>(target))
        return std::move(handleBinary(binary));
    else {
//        warns() << "Unexpected instruction in split: " << util::toString(*target) << endl;
        return {};
    }
}

#define SPLIT_EQ(lhvDomain, rhvDomain) \
    values[lhv] = lhvDomain->splitByEq(rhvDomain); \
    values[rhv] = rhvDomain->splitByEq(lhvDomain);

#define SPLIT_NEQ(lhvDomain, rhvDomain) \
    values[lhv] = lhvDomain->splitByEq(rhvDomain).swap(); \
    values[rhv] = rhvDomain->splitByEq(lhvDomain).swap();

#define SPLIT_LESS(lhvDomain, rhvDomain) \
    values[lhv] = lhvDomain->splitByLess(rhvDomain); \
    values[rhv] = rhvDomain->splitByLess(lhvDomain).swap();

#define SPLIT_GREATER(lhvDomain, rhvDomain) \
    values[rhv] = rhvDomain->splitByLess(lhvDomain); \
    values[lhv] = lhvDomain->splitByLess(rhvDomain).swap();

std::unordered_map<DomainStorage::Variable, Split> DomainStorage::handleIcmp(const llvm::ICmpInst* target) const {
    auto* lhv = target->getOperand(0);
    auto* rhv = target->getOperand(1);

    auto&& lhvDomain = get(lhv);
    auto&& rhvDomain = get(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in icmp splitter");

    std::unordered_map<Variable, Split> values;

    auto&& predicate = target->getPredicate();
    switch (predicate) {
        case llvm::CmpInst::ICMP_EQ:
        SPLIT_EQ(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::ICMP_NE:
        SPLIT_NEQ(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::ICMP_ULT:
        case llvm::CmpInst::ICMP_ULE:
            if (lhv->getType()->isPointerTy() && rhv->getType()->isPointerTy())
                break;
            SPLIT_LESS(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::ICMP_UGT:
        case llvm::CmpInst::ICMP_UGE:
            if (lhv->getType()->isPointerTy() && rhv->getType()->isPointerTy())
                break;
            SPLIT_GREATER(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::ICMP_SLT:
        case llvm::CmpInst::ICMP_SLE:
            values[lhv] = lhvDomain->splitByLess(rhvDomain);
            values[rhv] = rhvDomain->splitByLess(lhvDomain).swap();
            break;

        case llvm::CmpInst::ICMP_SGT:
        case llvm::CmpInst::ICMP_SGE:
            values[rhv] = rhvDomain->splitByLess(lhvDomain);
            values[lhv] = lhvDomain->splitByLess(rhvDomain).swap();
            break;

        default:
            UNREACHABLE("Unknown operation in icmp");
    }

    return std::move(values);
}

std::unordered_map<DomainStorage::Variable, Split> DomainStorage::handleFcmp(const llvm::FCmpInst* target) const {
    auto* lhv = target->getOperand(0);
    auto* rhv = target->getOperand(1);

    auto&& lhvDomain = get(lhv);
    auto&& rhvDomain = get(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in icmp splitter");

    std::unordered_map<Variable, Split> values;

    auto&& predicate = target->getPredicate();
    switch (predicate) {
        case llvm::CmpInst::FCMP_OEQ:
        case llvm::CmpInst::FCMP_UEQ:
        SPLIT_EQ(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::FCMP_ONE:
        case llvm::CmpInst::FCMP_UNE:
        SPLIT_NEQ(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::FCMP_OGT:
        case llvm::CmpInst::FCMP_UGT:
        case llvm::CmpInst::FCMP_OGE:
        case llvm::CmpInst::FCMP_UGE:
        SPLIT_LESS(lhvDomain, rhvDomain);
            break;

        case llvm::CmpInst::FCMP_OLT:
        case llvm::CmpInst::FCMP_ULT:
        case llvm::CmpInst::FCMP_OLE:
        case llvm::CmpInst::FCMP_ULE:
        SPLIT_GREATER(lhvDomain, rhvDomain);
            break;

        default:
            UNREACHABLE("Unknown operation in fcmp");
    }

    return std::move(values);
}

std::unordered_map<DomainStorage::Variable, Split> DomainStorage::handleBinary(const llvm::BinaryOperator* target) const {
    auto* lhv = target->getOperand(0);
    auto* rhv = target->getOperand(1);

    auto&& lhvDomain = get(lhv);
    auto&& rhvDomain = get(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in icmp splitter");

    std::unordered_map<Variable, Split> values;

    auto&& andImpl = [](Split lhv, Split rhv) -> Split {
        return {lhv.true_->meet(rhv.true_), lhv.false_->join(rhv.false_)};
    };
    auto&& orImpl = [](Split lhv, Split rhv) -> Split {
        return {lhv.true_->join(rhv.true_), lhv.false_->join(rhv.false_)};
    };

    auto* lhvInst = llvm::dyn_cast<llvm::Instruction>(lhv);
    auto* rhvInst = llvm::dyn_cast<llvm::Instruction>(rhv);
    if (not lhvInst || not rhvInst) return {};

    auto lhvValues = std::move(handleInst(lhvInst));
    auto rhvValues = std::move(handleInst(rhvInst));

    for (auto&& it : lhvValues) {
        auto value = it.first;
        auto lhvSplit = it.second;
        auto&& rhvit = util::at(rhvValues, value);
        if (not rhvit) continue;
        auto rhvSplit = rhvit.getUnsafe();

        auto&& opcode = target->getOpcode();
        switch (opcode) {
            case llvm::Instruction::And:
                values[value] = andImpl(lhvSplit, rhvSplit);
                break;
            case llvm::Instruction::Or:
                values[value] = orImpl(lhvSplit, rhvSplit);
                break;
            case llvm::Instruction::Xor: {
                // XOR = (!lhv AND rhv) OR (lhv AND !rhv)
                auto temp1 = andImpl({lhvSplit.false_, lhvSplit.true_}, rhvSplit);
                auto temp2 = andImpl(lhvSplit, {rhvSplit.false_, rhvSplit.true_});
                values[value] = orImpl(temp1, temp2);
                break;
            }
            default:
                UNREACHABLE("Unknown binary operator");
        }
    }

    return std::move(values);
}

} // namespace ir
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

