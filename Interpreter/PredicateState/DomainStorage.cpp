//
// Created by abdullin on 2/14/19.
//

#include "Interpreter/Domain/AbstractFactory.hpp"
#include "DomainStorage.hpp"
#include "Interpreter/Domain/VariableFactory.hpp"
#include "Interpreter/Domain/Numerical/DoubleInterval.hpp"
#include "Interpreter/Domain/Numerical/IntervalDomain.hpp"
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
        warns() << "Unknown constant term: " << term->getName() << endl;
        return nullptr;
    }
}

template <typename IntervalT>
class IntervalDomainImpl : public IntervalDomain<IntervalT, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = Term::Ptr;
    using Self = IntervalDomainImpl<IntervalT>;
    using EnvT = typename IntervalDomain<IntervalT, Term::Ptr, TermHash, TermEqualsWType>::EnvT;

protected:

    const VariableFactory* vf_;

public:

    explicit IntervalDomainImpl(const VariableFactory* vf) : IntervalDomain<IntervalT, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf) {}
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

template <typename MachineInt>
class PointsToDomainImpl : public PointsToDomain<MachineInt, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = Term::Ptr;
    using Self = PointsToDomainImpl<MachineInt>;
    using EnvT = typename PointsToDomain<MachineInt, Term::Ptr, TermHash, TermEqualsWType>::EnvT;

protected:

    const VariableFactory* vf_;

public:

    explicit PointsToDomainImpl(const VariableFactory* vf) : PointsToDomain<MachineInt, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf) {}
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
};

template <typename AggregateT>
class AggregateDomainImpl : public AggregateDomain<AggregateT, Term::Ptr, TermHash, TermEqualsWType> {
public:

    using Ptr = AbstractDomain::Ptr;
    using Variable = Term::Ptr;
    using Self = AggregateDomainImpl<AggregateT>;
    using EnvT = typename AggregateDomain<AggregateT, Term::Ptr, TermHash, TermEqualsWType>::EnvT;

protected:

    const VariableFactory* vf_;

public:

    explicit AggregateDomainImpl(const VariableFactory* vf) : AggregateDomain<AggregateT, Term::Ptr, TermHash, TermEqualsWType>(), vf_(vf) {}
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

DomainStorage::DomainStorage(const VariableFactory* vf) :
        ObjectLevelLogging("domain"),
        vf_(vf),
        bools_(std::make_shared<impl_::IntervalDomainImpl<Interval<UIntT>>>(vf_)),
        ints_(std::make_shared<impl_::IntervalDomainImpl<DoubleInterval<SIntT, UIntT>>>(vf_)),
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

std::pair<DomainStorage::Ptr, DomainStorage::Ptr> DomainStorage::split(Variable condition) const {
    auto&& true_ = std::make_shared<DomainStorage>(*this);
    auto&& false_ = std::make_shared<DomainStorage>(*this);

    auto&& condDomain = get(condition);
    if (condDomain->isTop() || condDomain->isBottom())
        return std::make_pair(true_, false_);

    auto&& splitted = std::move(handleTerm(condition.get()));
    for (auto&& it : splitted) {
        true_->assign(it.first, it.second.true_);
        false_->assign(it.first, it.second.false_);
    }
    return std::make_pair(true_, false_);
}

DomainStorage::SplitMap DomainStorage::handleTerm(const Term* term) const {
    if (auto&& cmp = llvm::dyn_cast<CmpTerm>(term)) {
        return std::move(handleCmp(cmp));
    } else if (auto&& bin = llvm::dyn_cast<BinaryTerm>(term)) {
        return std::move(handleBinary(bin));
//    } else if (llvm::isa<type::Bool>(term->getType().get())) {
//        auto condDomain = get(term->shared_from_this());
//        return ;
    } else {
        errs() << "Unknown term in splitter: " << term->getName() << endl;
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

DomainStorage::SplitMap DomainStorage::handleCmp(const CmpTerm* target) const {
    auto&& lhv = target->getLhv();
    auto&& rhv = target->getRhv();

    auto&& lhvDomain = get(lhv);
    auto&& rhvDomain = get(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in icmp splitter");

    SplitMap values;

    bool isPtr = llvm::isa<type::Pointer>(lhv->getType().get()) || llvm::isa<type::Pointer>(rhv->getType().get());

    switch (target->getOpcode()) {
        case llvm::ConditionType::EQ: SPLIT_EQ(lhvDomain, rhvDomain); break;
        case llvm::ConditionType::NEQ: SPLIT_NEQ(lhvDomain, rhvDomain); break;
        case llvm::ConditionType::GT:
        case llvm::ConditionType::GE:
            if (isPtr) break;
            SPLIT_GREATER(lhvDomain, rhvDomain);
            break;
        case llvm::ConditionType::LT:
        case llvm::ConditionType::LE:
            if (isPtr) break;
            SPLIT_LESS(lhvDomain, rhvDomain);
            break;
        case llvm::ConditionType::UGT:
        case llvm::ConditionType::UGE:
            if (isPtr) break;
            SPLIT_GREATER(lhvDomain, rhvDomain);
            break;
        case llvm::ConditionType::ULT:
        case llvm::ConditionType::ULE:
            if (isPtr) break;
            SPLIT_LESS(lhvDomain, rhvDomain);
            break;
        case llvm::ConditionType::TRUE:
        case llvm::ConditionType::FALSE:
            break;
        default:
            UNREACHABLE("Unknown operation in cmp term");
    }

    return std::move(values);
}

DomainStorage::SplitMap DomainStorage::handleBinary(const BinaryTerm* target) const {
    auto&& lhv = target->getLhv();
    auto&& rhv = target->getRhv();

    auto&& lhvDomain = get(lhv);
    auto&& rhvDomain = get(rhv);
    ASSERT(lhvDomain && rhvDomain, "Unknown values in icmp splitter");

    SplitMap values;
    auto&& andImpl = [](Split lhv, Split rhv) -> Split {
        return {lhv.true_->meet(rhv.true_), lhv.false_->join(rhv.false_)};
    };
    auto&& orImpl = [](Split lhv, Split rhv) -> Split {
        return {lhv.true_->join(rhv.true_), lhv.false_->join(rhv.false_)};
    };

    auto lhvValues = std::move(handleTerm(lhv.get()));
    auto rhvValues = std::move(handleTerm(rhv.get()));

    for (auto&& it : lhvValues) {
        auto value = it.first;
        auto lhvSplit = it.second;
        auto&& rhvit = util::at(rhvValues, value);
        if (not rhvit) continue;
        auto rhvSplit = rhvit.getUnsafe();

        switch (target->getOpcode()) {
            case llvm::ArithType::LAND:
            case llvm::ArithType::BAND:
                values[value] = andImpl(lhvSplit, rhvSplit);
                break;
            case llvm::ArithType::LOR:
            case llvm::ArithType::BOR:
                values[value] = orImpl(lhvSplit, rhvSplit);
                break;
            case llvm::ArithType::XOR: {
                // XOR = (!lhv AND rhv) OR (lhv AND !rhv)
                auto&& temp1 = andImpl({lhvSplit.false_, lhvSplit.true_}, rhvSplit);
                auto&& temp2 = andImpl(lhvSplit, {rhvSplit.false_, rhvSplit.true_});
                values[value] = orImpl(temp1, temp2);
                break;
            }
            case llvm::ArithType::IMPLIES:
                // IMPL = (!lhv) OR (rhv)
                values[value] = orImpl(lhvSplit.swap(), rhvSplit);
                break;
            default:
                UNREACHABLE("Unexpected binary term in splitter");
        }
    }

    return std::move(values);
}

} // namespace ir
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

