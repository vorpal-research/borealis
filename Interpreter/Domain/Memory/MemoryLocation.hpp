//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_MEMORYLOCATION_HPP
#define BOREALIS_MEMORYLOCATION_HPP

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt>
class NullLocation;

template <typename MachineInt>
class ArrayLocation;

template <typename MachineInt>
class StructLocation;

template <typename MachineInt>
class MemoryLocation : public AbstractDomain {
public:

    using Ptr = AbstractDomain::Ptr;
    using IntervalT = Interval<MachineInt>;


    virtual std::unordered_set<Ptr> offsets() const = 0;

    virtual Ptr load(Ptr, Type::Ptr) const = 0;
    virtual void store(Ptr, Ptr) = 0;
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

    NullLocation() : AbstractDomain(class_tag(*this)), factory_(AbstractFactory::get()) {}

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

    NullLocation(const NullLocation&) = delete;
    NullLocation(NullLocation&&) = delete;
    NullLocation& operator=(const NullLocation&) = delete;
    NullLocation& operator=(NullLocation&&) = delete;

    static Ptr instance() {
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

    void meetWith(ConstPtr other) override {
        ASSERTC(llvm::isa<Self>(other.get()));
    }

    void widenWith(ConstPtr other) override {
        joinWith(other);
    }

    size_t hashCode() const override { return 42; }
    std::string toString() const override { return "null"; }

    Ptr load(Ptr, Type::Ptr type) const override {
        return factory_->top(type);
    }

    void store(Ptr, Ptr) override {}
};

template <typename MachineInt>
class ArrayLocation : public MemoryLocation<MachineInt> {
public:
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = ArrayLocation<MachineInt>;
    using IntervalT = Interval<MachineInt>;

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
            AbstractDomain(class_tag(*this)), base_(location), offset_(offset), factory_(AbstractFactory::get()) {
        static_assert(llvm::isa<ArrayDomain<MachineInt>>(base_.get()), "Trying to create array location with non-array base");
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

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->base_);
        this->offset_->meetWith(otherRaw->offset_);
    }

    void widenWith(ConstPtr other) override {
        joinWith(other);
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override { return this->base_->toString(); }

    Ptr load(Ptr interval, Type::Ptr type) const override {
        auto* baseRaw = llvm::dyn_cast<ArrayDomain<MachineInt>>(base_.get());
        ASSERTC(baseRaw);
        ASSERT(baseRaw->elementType()->equals(type.get()), "Trying to load different type than the actual location");
        return this->base_->load(interval + this->offset_);
    }

    void store(Ptr interval, Ptr value) {
        return this->base_->store(interval + this->offset_, value);
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
            : base_(base), offsets_({offset}), factory_(AbstractFactory::get()) {
        static_assert(llvm::isa<StructDomain<MachineInt>>(base_.get()), "Trying to create struct base with non-struct base");
    }

    StructLocation(Ptr base, Offsets offsets)
            : base_(base), offsets_(offsets), factory_(AbstractFactory::get()) {
        static_assert(llvm::isa<StructDomain<MachineInt>>(base_.get()), "Trying to create struct base with non-struct base");
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

    const Offsets& offsets() const override {
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

        return this->base_ == otherRaw->location_;
    }

    void joinWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->location_);
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        ASSERTC(this->base_ == otherRaw->location_);
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    void widenWith(ConstPtr other) override {
        joinWith(other);
    }

    size_t hashCode() const override { return this->base_->hashCode(); }
    std::string toString() const override { return this->base_->toString(); }

    Ptr load(Ptr interval, Type::Ptr) const override {
        // TODO: type checking
        return this->base_->load(interval + this->offset_);
    }

    void store(Ptr interval, Ptr value) {
        return this->base_->store(interval + this->offset_, value);
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_MEMORYLOCATION_HPP
