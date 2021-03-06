//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_MEMORYLOCATION_HPP
#define BOREALIS_MEMORYLOCATION_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"
#include "ArrayDomain.hpp"
#include "PointerDomain.hpp"
#include "StructDomain.hpp"
#include "Type/TypeUtils.h"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt>
class MemoryLocation : public AbstractDomain {
public:

    explicit MemoryLocation(id_t id) : AbstractDomain(id) {}

    using Ptr = AbstractDomain::Ptr;
    using IntervalT = Interval<MachineInt>;
    using OffsetSet = std::unordered_set<Ptr, AbstractDomainHash, AbstractDomainEquals>;

    virtual Ptr length() const = 0;
    virtual OffsetSet offsets() const = 0;

    virtual bool isNull() const { return false; }
};

template <typename MachineInt, typename FunctionT>
class FunctionLocation : public MemoryLocation<MachineInt> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = FunctionLocation<MachineInt, FunctionT>;
    using IntervalT = Interval<MachineInt>;
    using OffsetSet = typename MemoryLocation<MachineInt>::OffsetSet;

private:

    Ptr base_;

private:

    static Self* unwrap(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    static const Self* unwrap(ConstPtr other) {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

public:

    FunctionLocation(Ptr function) : MemoryLocation<MachineInt>(class_tag(*this)), base_(function) {
        ASSERT(llvm::isa<FunctionT>(base_.get()), "Trying to create function location with non-function base");
    }

    FunctionLocation(const FunctionLocation&) = default;
    FunctionLocation(FunctionLocation&&) = default;
    FunctionLocation& operator=(const FunctionLocation&) = default;
    FunctionLocation& operator=(FunctionLocation&&) = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr length() const override {
        return AbstractFactory::get()->getMachineInt(AbstractFactory::TOP);
    }

    OffsetSet offsets() const override {
        return {};
    }

    bool isTop() const override { return base_->isTop(); }
    bool isBottom() const override { return base_->isBottom(); }

    void setTop() override {
        base_->setTop();
    }

    void setBottom() override {
        base_->setBottom();
    }

    bool leq(ConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        if (not otherRaw) return false;

        return this->base_->equals(otherRaw->base_);
    }

    void joinWith(ConstPtr other) {
        if (this == other.get()) return;
        auto* otherRaw = unwrap(other);
        this->base_ = this->base_->join(otherRaw->base_);
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);
        this->base_ = this->base_->meet(otherRaw->base_);
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) {
        joinWith(other);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override { return this->base_->toString(); }

    Ptr load(Type::Ptr, Ptr) const override {
        auto* baseRaw = llvm::dyn_cast<FunctionT>(base_.get());
        ASSERTC(baseRaw);
        return this->base_;
    }
};

template <typename MachineInt>
class NullLocation : public MemoryLocation<MachineInt> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = NullLocation<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using OffsetSet = typename MemoryLocation<MachineInt>::OffsetSet;

private:

    AbstractFactory* factory_;

private:

    static Self* unwrap(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    static const Self* unwrap(ConstPtr other) {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

public:

    NullLocation() : MemoryLocation<MachineInt>(class_tag(*this)), factory_(AbstractFactory::get()) {}
    NullLocation(const NullLocation&) = default;
    NullLocation(NullLocation&&) = default;
    NullLocation& operator=(const NullLocation&) = default;
    NullLocation& operator=(NullLocation&&) = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static Ptr get() {
        static Ptr instance_ = std::make_shared<NullLocation>();
        return instance_;
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr length() const override {
        return AbstractFactory::get()->getMachineInt(AbstractFactory::TOP);
    }

    OffsetSet offsets() const override {
        return {};
    }

    bool isTop() const override { return false; }
    bool isBottom() const override { return false; }

    void setTop() override {}
    void setBottom() override {}

    bool leq(ConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(ConstPtr other) const override {
        return llvm::isa<Self>(other.get());
    }

    void joinWith(ConstPtr other) {
        ASSERTC(llvm::isa<Self>(other.get()));
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) {
        ASSERTC(llvm::isa<Self>(other.get()));
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) {
        joinWith(other);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return 42; }
    std::string toString() const override { return "null"; }

    bool isNull() const override { return true; }

    Ptr load(Type::Ptr type, Ptr) const override {
        return factory_->top(type);
    }

    void store(Ptr, Ptr) override {}

    Ptr gep(Type::Ptr type, const std::vector<Ptr>&) override {
        return factory_->top(type);
    }
};

template <typename MachineInt>
class ArrayLocation : public MemoryLocation<MachineInt> {
public:
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = ArrayLocation<MachineInt>;
    using BaseT = ArrayDomain<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using PointerT = PointerDomain<MachineInt>;
    using OffsetSet = typename MemoryLocation<MachineInt>::OffsetSet;

private:

    Ptr base_;
    Ptr offset_;
    AbstractFactory* factory_;

private:

    static Self* unwrap(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    static const Self* unwrap(ConstPtr other) {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    const BaseT* unwrapBase() const {
        auto* baseRaw = llvm::dyn_cast<const BaseT>(base_.get());
        ASSERTC(baseRaw);
        return baseRaw;
    }

public:

    ArrayLocation(Ptr location, Ptr offset) :
            MemoryLocation<MachineInt>(class_tag(*this)), base_(location), offset_(offset), factory_(AbstractFactory::get()) {
        ASSERT(llvm::isa<BaseT>(base_.get()), "Trying to create array location with non-array base");
    }

    ArrayLocation(const ArrayLocation&) = default;
    ArrayLocation(ArrayLocation&&) = default;
    ArrayLocation& operator=(const ArrayLocation&) = default;
    ArrayLocation& operator=(ArrayLocation&&) = default;
    ~ArrayLocation() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr base() const {
        return base_;
    }

    Ptr length() const override {
        return unwrapBase()->lengthDomain();
    }

    OffsetSet offsets() const override {
        return { this->offset_ };
    }

    bool isTop() const override { return this->base_->isTop(); }
    bool isBottom() const override { return this->base_->isBottom(); }

    void setTop() override {
        this->base_->setTop();
    }

    void setBottom() override {
        this->base_->setBottom();
    }

    bool leq(ConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        if (not otherRaw) return false;

        return this->base_->equals(otherRaw->base_);
    }

    void joinWith(ConstPtr other) {
        if (this == other.get()) return;
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_->equals(otherRaw->base_));
        this->offset_ = this->offset_->join(otherRaw->offset_);
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_->equals(otherRaw->base_));
        this->offset_ = this->offset_->meet(otherRaw->offset_);
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) {
        if (this == other.get()) return;
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_->equals(otherRaw->base_));
        this->offset_ = this->offset_->widen(otherRaw->offset_);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override {
        std::stringstream ss;
        ss << "{ " << offset_->toString() << " }" << base_->toString();
        return ss.str();
    }

    Ptr load(Type::Ptr type, Ptr interval) const override {
        auto* baseRaw = llvm::dyn_cast<ArrayDomain<MachineInt>>(base_.get());
        ASSERTC(baseRaw);
        return this->base_->load(type, interval + this->offset_);
    }

    void store(Ptr value, Ptr offset) override {
        return this->base_->store(value, offset + this->offset_);
    }

    Ptr gep(Type::Ptr type, const std::vector<Ptr>& offsets) override {
        if (this->isTop()) {
            return factory_->top(type);
        } else if (this->isBottom()) {
            return factory_->bottom(type);
        } else {
            std::vector<Ptr> off(offsets.begin(), offsets.end());
            off[0] = off[0] + offset_;
            return base_->gep(type, off);
        }
    }
};

template <typename MachineInt>
class StructLocation : public MemoryLocation<MachineInt> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = StructLocation<MachineInt>;
    using BaseT = StructDomain<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using OffsetSet = typename MemoryLocation<MachineInt>::OffsetSet;

private:

    Ptr base_;
    OffsetSet offsets_;
    AbstractFactory* factory_;

private:

    static Self* unwrap(Ptr other) {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    static const Self* unwrap(ConstPtr other) {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    const BaseT* unwrapBase() const {
        auto* baseRaw = llvm::dyn_cast<const BaseT>(base_.get());
        ASSERTC(baseRaw);
        return baseRaw;
    }

public:

    StructLocation(Ptr base, Ptr offset)
            : MemoryLocation<MachineInt>(class_tag(*this)), base_(base), offsets_({offset}), factory_(AbstractFactory::get()) {
        ASSERT(llvm::isa<BaseT>(base_.get()), "Trying to create struct base with non-struct base");
    }

    StructLocation(Ptr base, OffsetSet offsets)
            : MemoryLocation<MachineInt>(class_tag(*this)), base_(base), offsets_(offsets), factory_(AbstractFactory::get()) {
        ASSERT(llvm::isa<StructDomain<MachineInt>>(base_.get()), "Trying to create struct base with non-struct base");
    }

    StructLocation(const StructLocation&) = default;
    StructLocation(StructLocation&&) = default;
    StructLocation& operator=(const StructLocation&) = default;
    StructLocation& operator=(StructLocation&&) = default;
    ~StructLocation() override = default;

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Ptr base() const {
        return base_;
    }

    Ptr length() const override {
        return factory_->getMachineInt(unwrapBase()->size());
    }

    OffsetSet offsets() const override {
        return this->offsets_;
    }

    bool isTop() const override { return this->base_->isTop(); }
    bool isBottom() const override { return this->base_->isBottom(); }

    void setTop() override {
        this->base_->setTop();
    }

    void setBottom() override {
        this->base_->setBottom();
    }

    bool leq(ConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = llvm::dyn_cast<StructLocation>(other.get());
        if (not otherRaw) return false;

        return this->base_->equals(otherRaw->base_);
    }

    void joinWith(ConstPtr other) {
        if (this == other.get()) return;
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_->equals(otherRaw->base_));
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_->equals(otherRaw->base_));
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) {
        joinWith(other);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override {
        std::stringstream ss;
        ss << "{" << util::head(offsets_)->toString();
        for (auto&& it : util::viewContainer(offsets_).drop(1)) {
            ss << ", " << it->toString();
        }
        ss << "} " << base_->toString();
        return ss.str();
    }

    Ptr load(Type::Ptr type, Ptr interval) const override {
        Ptr result = factory_->bottom(type);
        for (auto&& offset : this->offsets_) {
            auto&& load = this->base_->load(type, interval + offset);
            result = result->join(load);
        }
        return result;
    }

    void store(Ptr value, Ptr interval) override {
        for (auto&& offset : this->offsets_) {
            this->base_->store(value, offset + interval);
        }
    }

    Ptr gep(Type::Ptr type, const std::vector<Ptr>& offsets) override {
        if (this->isTop()) {
            return factory_->top(type);
        } else if (this->isBottom()) {
            return factory_->bottom(type);
        } else {
            auto result = factory_->bottom(type);
            std::vector<Ptr> offsetCopy(offsets.begin(), offsets.end());
            auto zero = offsets[0];

            for (auto&& it : offsets_) {
                offsetCopy[0] = zero + it;
                result = result->join(base_->gep(type, offsetCopy));
            }
            return result;
        }
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_MEMORYLOCATION_HPP
