//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_MEMORYLOCATION_HPP
#define BOREALIS_MEMORYLOCATION_HPP

#include "Interpreter/Domain/DomainFactory.h"

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

    virtual std::unordered_set<Ptr> offsets() const = 0;

    virtual bool isNull() const { return false; }
};

template <typename MachineInt>
class NullLocation : public MemoryLocation<MachineInt> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = NullLocation<MachineInt>;
    using IntervalT = Interval<MachineInt>;

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

    std::unordered_set<Ptr> offsets() const override {
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

    void joinWith(ConstPtr other) override {
        ASSERTC(llvm::isa<Self>(other.get()));
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) override {
        ASSERTC(llvm::isa<Self>(other.get()));
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) override {
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
    using IntervalT = Interval<MachineInt>;
    using PointerT = Pointer<MachineInt>;

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

public:

    ArrayLocation(Ptr location, Ptr offset) :
            MemoryLocation<MachineInt>(class_tag(*this)), base_(location), offset_(offset), factory_(AbstractFactory::get()) {
        ASSERT(llvm::isa<ArrayDomain<MachineInt>>(base_.get()), "Trying to create array location with non-array base");
    }

    ArrayLocation(const ArrayLocation&) = default;
    ArrayLocation(ArrayLocation&&) = default;
    ArrayLocation& operator=(const ArrayLocation&) = default;
    ArrayLocation& operator=(ArrayLocation&&) = default;
    ~ArrayLocation() override = default;

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    std::unordered_set<Ptr> offsets() const override {
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

        return this->base_ == otherRaw->base_;
    }

    void joinWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->base_);
        this->offset_->joinWith(otherRaw->offset_);
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->base_);
        this->offset_->meetWith(otherRaw->offset_);
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) override {
        joinWith(other);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override { return this->base_->toString(); }

    Ptr load(Type::Ptr type, Ptr interval) const override {
        auto* baseRaw = llvm::dyn_cast<ArrayDomain<MachineInt>>(base_.get());
        ASSERTC(baseRaw);
        return this->base_->load(type, interval + this->offset_);
    }

    void store(Ptr value, Ptr offset) override {
        return this->base_->store(offset + this->offset_, value);
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
    using IntervalT = Interval<MachineInt>;
    using Offsets = std::unordered_set<Ptr>;

private:

    Ptr base_;
    Offsets offsets_;
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

    StructLocation(Ptr base, Ptr offset)
            : MemoryLocation<MachineInt>(class_tag(*this)), base_(base), offsets_({offset}), factory_(AbstractFactory::get()) {
        ASSERT(llvm::isa<StructDomain<MachineInt>>(base_.get()), "Trying to create struct base with non-struct base");
    }

    StructLocation(Ptr base, Offsets offsets)
            : MemoryLocation<MachineInt>(class_tag(*this)), base_(base), offsets_(offsets), factory_(AbstractFactory::get()) {
        ASSERT(llvm::isa<StructDomain<MachineInt>>(base_.get()), "Trying to create struct base with non-struct base");
    }

    StructLocation(const StructLocation&) = default;
    StructLocation(StructLocation&&) = default;
    StructLocation& operator=(const StructLocation&) = default;
    StructLocation& operator=(StructLocation&&) = default;
    ~StructLocation() override = default;

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    Offsets offsets() const override {
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

        return this->base_ == otherRaw->base_;
    }

    void joinWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->base_);
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->base_);
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    void widenWith(ConstPtr other) override {
        joinWith(other);
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override { return this->base_->toString(); }

    Ptr load(Type::Ptr type, Ptr interval) const override {
        Ptr result = factory_->bottom(type);
        for (auto&& offset : this->offsets_) {
            auto&& load = this->base_->load(type, interval + offset);
            result->joinWith(load);
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
                result->joinWith(base_->gep(type, offsetCopy));
            }
            return result;
        }
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_MEMORYLOCATION_HPP
