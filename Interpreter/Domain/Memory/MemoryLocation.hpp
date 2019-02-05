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

template <typename MachineInt, typename Element>
class ArrayLocation;

template <typename MachineInt, typename ...Element>
class StructLocation;

template <typename MachineInt>
class MemoryLocation : public AbstractDomain<MemoryLocation<MachineInt>> {
public:
    enum Kind {
        ARRAY,
        STRUCT,
        NULL_
    };

private:

    Kind kind_;

public:

    using Ptr = std::shared_ptr<MemoryLocation<MachineInt>>;
    using ConstPtr = std::shared_ptr<const MemoryLocation<MachineInt>>;

    using IntervalT = Interval<MachineInt>;
    using IntervalPtr = typename IntervalT::Ptr;

public:

    MemoryLocation() : kind_(ARRAY) {}
    explicit MemoryLocation(Kind kind) : kind_(kind) {}
    ~MemoryLocation() override = default;

    template <typename MInt>
    static std::shared_ptr<MemoryLocation<MInt>> makeNull() {
        return NullLocation<MInt>::instance();
    }

    template <typename MInt, typename Element>
    static std::shared_ptr<MemoryLocation<MInt>> makeArray(
            typename ArrayLocation<MInt, Element>::LocPtr ptr,
            typename ArrayLocation<MInt, Element>::IntervalPtr offset) {
        return std::make_shared<ArrayLocation<MInt, Element>>(ptr, offset);
    }

    template <typename MInt, typename ...Element>
    static std::shared_ptr<MemoryLocation<MInt>> makeStruct(
            typename StructLocation<MInt, Element...>::LocPtr ptr,
            typename StructLocation<MInt, Element...>::IntervalPtr offset) {
        return std::make_shared<StructLocation<MInt, Element...>>(ptr, offset);
    }

    template <typename MInt, typename ...Element>
    static std::shared_ptr<MemoryLocation<MInt>> makeStruct(
            typename StructLocation<MInt, Element...>::LocPtr ptr,
            const typename StructLocation<MInt, Element...>::Offsets& offsets) {
        return std::make_shared<StructLocation<MInt, Element...>>(ptr, offsets);
    }

    bool kind() const { return this->kind_; }
    virtual const std::unordered_set<IntervalPtr>& offsets() const = 0;

    bool isArray() const { return this->kind_ == ARRAY; }
    bool isStruct() const { return this->kind_ == STRUCT; }
    bool isNull() const { return this->kind_ == NULL_; }

    static bool classof(const MemoryLocation*) {
        return true;
    }

    template <typename Elem>
    typename Elem::Ptr load(IntervalPtr interval) const {
        if (this->isArray()) {
            return llvm::dyn_cast<ArrayLocation<MachineInt, Elem>>(this)->load(interval);
        } else if (this->isStruct()) {
            return nullptr;
        } else if (this->isNull()) {
            return llvm::dyn_cast<NullLocation<MachineInt>>(this)->load(interval);
        } else {
            UNREACHABLE("Unknown location type");
        }
    }

    template <typename Elem>
    void store(IntervalPtr, typename Elem::Ptr) {}

};

template <typename MachineInt>
class NullLocation : public MemoryLocation<MachineInt> {
public:

    using Base = MemoryLocation<MachineInt>;
    using BasePtr = typename Base::Ptr;
    using BaseConstPtr = typename Base::ConstPtr;

    using IntervalT = Interval<MachineInt>;
    using IntervalPtr = typename IntervalT::Ptr;

private:

    NullLocation() = default;

public:

    NullLocation(const NullLocation&) = delete;
    NullLocation(NullLocation&&) = delete;
    NullLocation& operator=(const NullLocation&) = delete;
    NullLocation& operator=(NullLocation&&) = delete;

    static BasePtr instance() {
        static BasePtr instance_ = std::make_shared<NullLocation>();
        return instance_;
    }

    static bool classof(const Base* other) {
        return other->kind() == Base::NULL_;
    }

    const std::unordered_set<IntervalPtr>& offsets() const override {
        return {};
    }

    bool isTop() const override { return false; }
    bool isBottom() const override { return false; }

    void setTop() override {}
    void setBottom() override {}

    bool leq(BaseConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(BaseConstPtr other) const override {
        return other->isNull();
    }

    void joinWith(BaseConstPtr other) override {
        ASSERTC(other->isNull());
    }

    void meetWith(BaseConstPtr other) override {
        ASSERTC(other->isNull());
    }

    void widenWith(BaseConstPtr other) override {
        joinWith(other);
    }

    size_t hashCode() const override { return 42; }
    std::string toString() const override { return "null"; }

    template <typename Elem>
    typename Elem::Ptr load(IntervalPtr) const {
        return Elem::top();
    }

    template <typename Elem>
    void store(IntervalPtr, typename Elem::Ptr) {}
};

template <typename MachineInt, typename Element>
class ArrayLocation : public MemoryLocation<MachineInt> {
public:

    using Base = MemoryLocation<MachineInt>;
    using BasePtr = typename Base::Ptr;
    using BaseConstPtr = typename Base::ConstPtr;
    using IntervalT = Interval<MachineInt>;
    using IntervalPtr = typename IntervalT::Ptr;
    using LocPtr = typename ArrayDomain<MachineInt, Element>::Ptr;

private:

    LocPtr location_;
    IntervalPtr offset_;

public:

    ArrayLocation(LocPtr location, IntervalPtr offset) : Base(Base::ARRAY), location_(location), offset_(offset) {}
    ArrayLocation(const ArrayLocation&) = default;
    ArrayLocation(ArrayLocation&&) = default;
    ArrayLocation& operator=(const ArrayLocation&) = default;
    ArrayLocation& operator=(ArrayLocation&&) = default;
    ~ArrayLocation() override = default;

    static bool classof(const Base* other) {
        return other->kind() == Base::ARRAY;
    }

    const std::unordered_set<IntervalPtr>& offsets() const override {
        return {this->offset_};
    }

    bool isTop() const override { return this->location_->isTop(); }
    bool isBottom() const override { return this->location_->isBottom(); }

    void setTop() override {
        this->location_->setTop();
    }

    void setBottom() override {
        this->location_->setBottom();
    }

    bool leq(BaseConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(BaseConstPtr other) const override {
        if (not other->isArray()) return false;

        auto&& otherRaw = llvm::dyn_cast<ArrayLocation>(other.get());
        return this->location_ == otherRaw->location_;
    }

    void joinWith(BaseConstPtr other) override {
        ASSERTC(other->isArray());
        auto&& otherRaw = llvm::dyn_cast<ArrayLocation>(other.get());

        ASSERTC(this->location_ == otherRaw->location_);
        this->offset_->joinWith(otherRaw->offset_);
    }

    void meetWith(BaseConstPtr other) override {
        ASSERTC(other->isArray());
        auto&& otherRaw = llvm::dyn_cast<ArrayLocation>(other.get());

        ASSERTC(this->location_ == otherRaw->location_);
        this->offset_->meetWith(otherRaw->offset_);
    }

    void widenWith(BaseConstPtr other) override {
        joinWith(other);
    }

    size_t hashCode() const override { return this->location_->hashCode(); }
    std::string toString() const override { return this->location_->toString(); }

    template <typename Elem>
    typename Elem::Ptr load(IntervalPtr interval) const {
        return this->location_->load(interval + this->offset_);
    }

    template <typename Elem>
    void store(IntervalPtr interval, typename Elem::Ptr value) {
        return this->location_->store(interval + this->offset_, value);
    }
};

template <typename MachineInt, typename ...Element>
class StructLocation : public MemoryLocation<MachineInt> {
public:

    using Base = MemoryLocation<MachineInt>;
    using BasePtr = typename Base::Ptr;
    using BaseConstPtr = typename Base::ConstPtr;
    using IntervalT = Interval<MachineInt>;
    using IntervalPtr = typename IntervalT::Ptr;
    using Offsets = std::unordered_set<IntervalPtr>;
    using LocPtr = typename StructDomain<MachineInt, Element...>::Ptr;

private:

    LocPtr location_;
    Offsets offsets_;

public:

    StructLocation(LocPtr location, IntervalPtr offset)
            : location_(location), offsets_({offset}) {}

    StructLocation(LocPtr location, Offsets offsets)
            : location_(location), offsets_(offsets) {}
    StructLocation(const StructLocation&) = default;
    StructLocation(StructLocation&&) = default;
    StructLocation& operator=(const StructLocation&) = default;
    StructLocation& operator=(StructLocation&&) = default;
    ~StructLocation() override = default;

    static bool classof(const Base* other) {
        return other->kind() == Base::STRUCT;
    }

    const std::unordered_set<IntervalPtr>& offsets() const override {
        return this->offsets_;
    }

    bool isTop() const override { return this->location_->isTop(); }
    bool isBottom() const override { return this->location_->isBottom(); }

    void setTop() override {
        this->location_->setTop();
    }

    void setBottom() override {
        this->location_->setBottom();
    }

    bool leq(BaseConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(BaseConstPtr other) const override {
        if (not other->isStruct()) return false;

        auto&& otherRaw = llvm::dyn_cast<StructLocation>(other.get());
        return this->location_ == otherRaw->location_;
    }

    void joinWith(BaseConstPtr other) override {
        ASSERTC(other->isStruct());
        auto&& otherRaw = llvm::dyn_cast<StructLocation>(other.get());

        ASSERTC(this->location_ == otherRaw->location_);
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    void meetWith(BaseConstPtr other) override {
        ASSERTC(other->isStruct());
        auto&& otherRaw = llvm::dyn_cast<StructLocation>(other.get());

        ASSERTC(this->location_ == otherRaw->location_);
        for (auto&& offset : otherRaw->offsets_) {
            this->offsets_.insert(offset);
        }
    }

    void widenWith(BaseConstPtr other) override {
        joinWith(other);
    }

    size_t hashCode() const override { return this->location_->hashCode(); }
    std::string toString() const override { return this->location_->toString(); }

    template <typename Elem>
    typename Elem::Ptr load(IntervalPtr interval) const {
        return this->location_->load(interval + this->offset_);
    }

    template <typename Elem>
    void store(IntervalPtr interval, typename Elem::Ptr value) {
        return this->location_->store(interval + this->offset_, value);
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_MEMORYLOCATION_HPP
