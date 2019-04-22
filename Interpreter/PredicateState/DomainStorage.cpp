//
// Created by abdullin on 2/14/19.
//

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

#include "Term/OpaqueBoolConstantTerm.h"
#include "Term/OpaqueBigIntConstantTerm.h"
#include "Term/OpaqueFloatingConstantTerm.h"
#include "Term/OpaqueIntConstantTerm.h"
#include "Term/OpaqueInvalidPtrTerm.h"
#include "Term/OpaqueNullPtrTerm.h"
#include "Term/OpaqueStringConstantTerm.h"
#include "Term/OpaqueUndefTerm.h"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace ps {

namespace impl_ {

AbstractDomain::Ptr getConstant(const VariableFactory* vf, Term::Ptr term) {
    if (auto* obic = llvm::dyn_cast<OpaqueBigIntConstantTerm>(term.get())) {
        auto intTy = llvm::cast<type::Integer>(term->getType().get());
        auto apint = llvm::APInt(intTy->getBitsize(), obic->getRepresentation(), 10);
        return vf->af()->getInteger(*apint.getRawData(), apint.getBitWidth());

    } else if (auto* obc = llvm::dyn_cast<OpaqueBoolConstantTerm>(term.get())) {
        return vf->af()->getBool(obc->getValue());

    } else if (auto* ofc = llvm::dyn_cast<OpaqueFloatingConstantTerm>(term.get())) {
        return vf->af()->getFloat(ofc->getValue());

    } else if (auto* oic = llvm::dyn_cast<OpaqueIntConstantTerm>(term.get())) {
        if (auto intTy = llvm::dyn_cast<type::Integer>(term->getType().get())) {
            return vf->af()->getInteger(oic->getValue(), intTy->getBitsize());

        } else if (llvm::isa<type::Pointer>(term->getType().get())) {
            return (oic->getValue() == 0) ? vf->af()->getNullptr() : vf->top(term->getType());
        } else {

            warns() << "Unknown type in OpaqueIntConstant: " << TypeUtils::toString(*term->getType().get()) << endl;
            return vf->af()->getMachineInt(oic->getValue());
        };

    } else if (llvm::isa<OpaqueInvalidPtrTerm>(term.get())) {
        return vf->af()->getNullptr();

    } else if (llvm::isa<OpaqueNullPtrTerm>(term.get())) {
        return vf->af()->getNullptr();

    } else if (auto* osc = llvm::dyn_cast<OpaqueStringConstantTerm>(term.get())) {
        if (auto array = llvm::dyn_cast<type::Array>(term->getType())) {
            std::vector<AbstractDomain::Ptr> elements;
            for (auto&& it : osc->getValue()) {
                elements.push_back(vf->af()->getInteger(it, 8));
            }
            return vf->af()->getArray(array->getElement(), elements);
        } else {
            return vf->top(term->getType());
        }

    } else if (llvm::isa<OpaqueUndefTerm>(term.get())) {
        return vf->top(term->getType());

    } else {
//        warns() << "Unknown constant term: " << term->getName() << endl;
        return nullptr;
    }
}

template <typename IntervalT>
class IntervalDomainImpl : public IntervalDomain<IntervalT, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = Term::Ptr;
    using Self = IntervalDomainImpl<IntervalT>;
    using ParentT = NumericalDomain<Variable>;
    using EnvT = typename IntervalDomain<IntervalT, Term::Ptr, TermHash, TermEqualsWType>::EnvT;

protected:

    const VariableFactory* vf_;
    const Ptr input_;

private:

    const ParentT* unwrapInput() const {
        auto* ptr = llvm::dyn_cast<ParentT>(input_.get());
        ASSERTC(ptr);
        return ptr;
    }

public:

    explicit IntervalDomainImpl(const VariableFactory* vf, const Ptr input)
            : IntervalDomain<IntervalT, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf), input_(input) {}
    IntervalDomainImpl(const IntervalDomainImpl&) = default;
    IntervalDomainImpl(IntervalDomainImpl&&) = default;
    IntervalDomainImpl& operator=(const IntervalDomainImpl&) = default;
    IntervalDomainImpl& operator=(IntervalDomainImpl&&) = default;
    ~IntervalDomainImpl() override = default;

    static Ptr top() { return std::make_shared<Self>(EnvT::top()); }
    static Ptr bottom() { return std::make_shared<Self>(EnvT::bottom()); }

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
        if (input_ != nullptr) {
            auto inputValue = unwrapInput()->get(x);
            if (inputValue != nullptr) return inputValue;
        }

        if (auto&& local = this->unwrapEnv()->get(x,
                [&]() -> AbstractDomain::Ptr { return vf_->bottom(x->getType()); },
                [&]() -> AbstractDomain::Ptr { return vf_->top(x->getType()); }
        )) {

            return local;

        } else if (auto&& constant = getConstant(vf_, x)) {
            return constant;
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
class OctagonDomainImpl : public OctagonDomain<N1, N2, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;
    using Variable = Term::Ptr;
    using DOctagon = DoubleOctagon<N1, N2, Term::Ptr, TermHash, TermEqualsWType>;
    using OctagonMap = std::unordered_map<size_t, Ptr>;
    using Self = OctagonDomainImpl<N1, N2>;
    using ParentT = NumericalDomain<Variable>;

protected:

    const VariableFactory* vf_;
    const Ptr input_;

protected:

    const ParentT* unwrapInput() const {
        auto* ptr = llvm::dyn_cast<ParentT>(input_.get());
        ASSERTC(ptr);
        return ptr;
    }

    size_t unwrapTypeSize(Variable x) const override {
        auto* integer = llvm::dyn_cast<type::Integer>(x->getType().get());
        ASSERTC(integer);

        return integer->getBitsize();
    }

    util::option<typename DOctagon::DNumber> unwrapDInterval(const typename DOctagon::DInterval* interval) const {
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

    util::option<typename DOctagon::DNumber> getConstant(Variable x) const {
        if (auto&& constant = impl_::getConstant(vf_, x)) {
            auto* domainRaw = llvm::dyn_cast<typename DOctagon::DInterval>(constant.get());
            ASSERTC(domainRaw);

            return unwrapDInterval(domainRaw);
        } else {
            return util::nothing();
        }
    }

public:

    explicit OctagonDomainImpl(const VariableFactory* vf, const Ptr input)
        : OctagonDomain<N1, N2, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf), input_(input) {}
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
        if (input_ != nullptr and unwrapInput()->contains(x)) {
            return unwrapInput()->get(x);

        }
        if (auto&& constant = impl_::getConstant(vf_, x)) {
            return constant;

        } else {
            auto bitsize = unwrapTypeSize(x);
            auto* octagon = this->unwrapOctagon(bitsize);

            return octagon->get(x);
        }
    }

    bool equals(ConstPtr other) const override {
        // This is generally fucked up, but for octagons it should be this way
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
class PointsToDomainImpl : public PointsToDomain<MachineInt, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = Term::Ptr;
    using Self = PointsToDomainImpl<MachineInt>;
    using ParentT = MemoryDomain<MachineInt, Variable>;
    using EnvT = typename PointsToDomain<MachineInt, Term::Ptr, TermHash, TermEqualsWType>::EnvT;

protected:

    const VariableFactory* vf_;
    const Ptr input_;

private:

    const ParentT* unwrapInput() const {
        auto* ptr = llvm::dyn_cast<ParentT>(input_.get());
        ASSERTC(ptr);
        return ptr;
    }

public:

    explicit PointsToDomainImpl(const VariableFactory* vf, const Ptr input)
            : PointsToDomain<MachineInt, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf), input_(input) {}
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

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr get(Variable x) const override {
        if (input_ != nullptr) {
            auto inputValue = unwrapInput()->get(x);
            if (inputValue != nullptr) return inputValue;
        }

        if (auto&& local = this->unwrapEnv()->get(x,
                [&]() -> AbstractDomain::Ptr { return PointerDomain<MachineInt>::bottom(); },
                [&]() -> AbstractDomain::Ptr { return PointerDomain<MachineInt>::top(); }
        )) {
            return local;

        } else if (auto&& constant = getConstant(vf_, x)) {
            return constant;
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
class AggregateDomainImpl : public AggregateDomain<AggregateT, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = Term::Ptr;
    using Self = AggregateDomainImpl<AggregateT>;
    using ParentT = Aggregate<Variable>;
    using EnvT = typename AggregateDomain<AggregateT, Term::Ptr, TermHash, TermEqualsWType>::EnvT;

protected:

    const VariableFactory* vf_;
    const Ptr input_;

private:

    const ParentT* unwrapInput() const {
        auto* ptr = llvm::dyn_cast<ParentT>(input_.get());
        ASSERTC(ptr);
        return ptr;
    }

public:

    explicit AggregateDomainImpl(const VariableFactory* vf, const Ptr input)
            : AggregateDomain<AggregateT, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf), input_(input) {}
    AggregateDomainImpl(const AggregateDomainImpl&) = default;
    AggregateDomainImpl(AggregateDomainImpl&&) = default;
    AggregateDomainImpl& operator=(const AggregateDomainImpl&) = default;
    AggregateDomainImpl& operator=(AggregateDomainImpl&&) = default;
    ~AggregateDomainImpl() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static Ptr top() { return std::make_shared(EnvT::top()); }
    static Ptr bottom() { return std::make_shared(EnvT::bottom()); }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr get(Variable x) const override {
        if (input_ != nullptr) {
            auto inputValue = unwrapInput()->get(x);
            if (inputValue != nullptr) return inputValue;
        }

        if (auto&& local = this->unwrapEnv()->get(x,
                [&]() -> AbstractDomain::Ptr { return vf_->bottom(x->getType()); },
                [&]() -> AbstractDomain::Ptr { return vf_->top(x->getType()); }
        )) {
            return local;

        } else if (auto&& constant = getConstant(vf_, x)) {
            return constant;
        }
        return nullptr;
    }
};

} // namespace impl_

static config::StringConfigEntry numericalDomain("absint", "numeric");

inline AbstractDomain::Ptr initNumericalDomain(const VariableFactory* vf, AbstractDomain::Ptr input) {
    auto&& numericalDomainName = numericalDomain.get("interval");

    using SIntT = typename DomainStorage::SIntT;
    using UIntT = typename DomainStorage::UIntT;

    if (numericalDomainName == "interval") {
        return std::make_shared<impl_::IntervalDomainImpl<DoubleInterval<SIntT, UIntT>>>(vf, input);
    } else if (numericalDomainName == "octagon") {
        return std::make_shared<impl_::OctagonDomainImpl<SIntT, UIntT>>(vf, input);
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

DomainStorage::DomainStorage(const VariableFactory* vf, DomainStorage::Ptr input) :
        ObjectLevelLogging("domain"),
        vf_(vf),
        input_(input),
        bools_(std::make_shared<impl_::IntervalDomainImpl<Interval<UIntT>>>(vf_, (input_ ? input_->bools_ : nullptr))),
        ints_(initNumericalDomain(vf_, (input_ ? input_->ints_ : nullptr))),
        floats_(std::make_shared<impl_::IntervalDomainImpl<Interval<Float>>>(vf_, (input_ ? input_->floats_ : nullptr))),
        memory_(std::make_shared<impl_::PointsToDomainImpl<MachineIntT>>(vf_, (input_ ? input_->memory_ : nullptr))),
        structs_(std::make_shared<impl_::AggregateDomainImpl<StructDomain<MachineIntT>>>(vf_, (input_ ? input_->structs_ : nullptr))) {}

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
    auto&& type = x->getType();

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
    else if (llvm::isa<type::UnknownType>(type.get()) and llvm::isa<OpaqueNullPtrTerm>(x.get()))
        return unwrapMemory()->get(x);
    else {
        return nullptr;
    }
}

void DomainStorage::assign(Variable x, Variable y) const {
    assign(x, get(y));
}

void DomainStorage::assign(Variable x, AbstractDomain::Ptr domain) const {
    auto&& type = x->getType();

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
void DomainStorage::apply(llvm::ArithType op, Variable x, Variable y, Variable z) {
    auto&& type = y->getType();
    if (llvm::isa<type::Bool>(type.get())) {
        auto* bools = unwrapBool();
        bools->applyTo(op, x, y, z);

    } else if (llvm::isa<type::Integer>(type.get())){
        auto* ints = unwrapInt();
        ints->applyTo(op, x, y, z);
    } else if (llvm::isa<type::Float>(type.get())) {
        auto* fts = unwrapFloat();
        fts->applyTo(op, x, y, z);
    } else {
        UNREACHABLE("Binary operation on unknown variable types")
    }
}

/// x = y op z
void DomainStorage::apply(llvm::ConditionType op, Variable x, Variable y, Variable z) {
    auto* bools = unwrapBool();

    AbstractDomain::Ptr xd;
    if (llvm::isa<type::Pointer>(y->getType().get()) && llvm::isa<type::Pointer>(z->getType().get())) {
        auto* memory = unwrapMemory();
        xd = memory->applyTo(op, y, z);

    } else if (llvm::isa<type::Integer>(y->getType().get()) && llvm::isa<type::Integer>(z->getType().get())) {
        auto* ints = unwrapInt();
        xd = ints->applyTo(op, y, z);

    } else if (llvm::isa<type::Float>(y->getType().get()) && llvm::isa<type::Float>(z->getType().get())) {
        auto* floats = unwrapFloat();
        xd = floats->applyTo(op, y, z);

    } else {
        UNREACHABLE("Unreachable!");
    }

    bools->assign(x, xd);
}

void DomainStorage::apply(CastOperator op, Variable x, Variable y) {
    auto&& yDom = get(y);
    auto&& targetType = x->getType();

    auto&& xDom = vf_->af()->cast(op, targetType, yDom);
    assign(x, xDom);
}

/// x = *ptr
void DomainStorage::load(Variable x, Variable ptr) {
    auto&& xDom = unwrapMemory()->loadFrom(x->getType(), ptr);
    assign(x, xDom);
}

/// *ptr = x
void DomainStorage::store(Variable ptr, Variable x) {
    unwrapMemory()->storeTo(ptr, get(x));
}

void DomainStorage::storeWithWidening(DomainStorage::Variable ptr, DomainStorage::Variable x) {
    auto&& ptrDom = unwrapMemory()->loadFrom(x->getType(), ptr);
    auto&& xDom = get(x);
    unwrapMemory()->storeTo(ptr, ptrDom->widen(xDom));
}

/// x = gep(ptr, shifts)
void DomainStorage::gep(Variable x, Variable ptr, const std::vector<Variable>& shifts) {
    std::vector<AbstractDomain::Ptr> normalizedShifts;
    for (auto&& it : shifts) {
        normalizedShifts.emplace_back(vf_->af()->machineIntInterval(get(it)));
    }
    unwrapMemory()->gepFrom(x, x->getType(), ptr, normalizedShifts);
}

/// x = extract(struct, index)
void DomainStorage::extract(Variable x, Variable structure, Variable index) {
    auto&& normalizedIndex = vf_->af()->machineIntInterval(get(index));
    auto&& xDom = unwrapStruct()->extractFrom(x->getType(), structure, normalizedIndex);
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
        auto&& arrayType = tf->getArray(llvm::cast<type::Pointer>(x->getType().get())->getPointed(), bounds.second.number());
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

} // namespace ir
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

