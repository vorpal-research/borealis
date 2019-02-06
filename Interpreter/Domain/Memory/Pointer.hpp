//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_POINTER_HPP
#define BOREALIS_POINTER_HPP

#include "Interpreter/Domain/Domain.h"
#include "MemoryLocation.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt>
class Pointer : public AbstractDomain {
public:

    using Self = Pointer<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using PointsToSet = std::unordered_set<Ptr>;

private:

    Type::Ptr pointedType_;
    bool isBottom_;
    AbstractFactory* factory_;
    PointsToSet ptsTo_;

private:
    struct TopTag{};
    struct BottomTag{};

    explicit Pointer(TopTag, Type::Ptr type) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(false), factory_(AbstractFactory::get()) {}
    explicit Pointer(BottomTag, Type::Ptr type) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(true), factory_(AbstractFactory::get()) {}

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

    explicit Pointer(Type::Ptr type) : Pointer(TopTag{}, type) {}
    Pointer(Type::Ptr type, const PointsToSet& ptsTo) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(false), factory_(AbstractFactory::get()), ptsTo_(ptsTo) {}
    Pointer(const Pointer&) = default;
    Pointer(Pointer&&) = default;
    Pointer& operator=(const Pointer&) = default;
    Pointer& operator=(Pointer&&) = default;
    ~Pointer() override = default;

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    static Ptr top(Type::Ptr type) { return Pointer(TopTag{}, type); }
    static Ptr bottom(Type::Ptr type) { return Pointer(BottomTag{}, type); }

    bool isTop() const override { return (not this->isBottom_) and this->ptsTo_.empty(); }
    bool isBottom() const override { return this->isBottom_ and this->ptsTo_.empty(); }

    bool isNull() const { return this->ptsTo_.size() == 1 and this->ptsTo_.begin()->isNull(); }
    bool pointsToNull() const {
        for (auto&& it : this->ptsTo_) {
            if (it->isNull()) return true;
        }
        return false;
    }

    void setTop() override {
        this->isBottom_ = false;
        this->ptsTo_.clear();
    }

    void setBottom() override {
        this->isBottom_ = true;
        this->ptsTo_.clear();
    }

    bool leq(ConstPtr) const override {
        UNREACHABLE("Unimplemented");
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isTop()) {
            return other->isTop();
        } else if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isTop() || other->isBottom()) {
            return false;
        } else if (this->isNull()) {
            return otherRaw->isNull();
        } else {

            if (this->ptsTo_.size() != otherRaw->ptsTo_.size()) return false;

            for (auto&& it : otherRaw->ptsTo_) {
                auto&& its = this->ptsTo_.find(it);
                if (its == this->ptsTo_.end()) return false;
                if (not util::equal_with_find(it->offsets(), its->offsets(), LAM(a, a), LAM2(a, b, a->equals(b))))
                    return false;
            }
            return true;
        }
    }

    void joinWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else if (this->isTop()) {
            return;
        } else if (other->isBottom()) {
            return;
        } else if (other->isTop()) {
            this->setTop();
        } else {

            for (auto&& it : otherRaw->ptsTo_) {
                auto&& its = this->ptsTo_.find(it);
                if (its == this->ptsTo_.end()) {
                    this->ptsTo_.insert(it);
                } else {
                    its->joinWith(it);
                }
            }
        }
    }

    void widenWith(ConstPtr other) override { joinWith(other); }

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (this->isTop()) {
            this->operator=(*otherRaw);
        } else if (other->isBottom()) {
            this->setBottom();
        } else if (other->isTop()) {
            return;
        } else {

            for (auto&& it : this->ptsTo_) {
                if (not util::contains(otherRaw->ptsTo_, it)) {
                    this->ptsTo_.erase(it);
                }
            }

            for (auto&& it : otherRaw->ptsTo_) {
                auto&& its = this->ptsTo_.find(it);
                if (its != this->ptsTo_.end()) {
                    its->meetWith(it);
                }
            }
        }
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(this->isBottom_, this->ptsTo_);
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << "Ptr [";
        if (this->isTop()) ss << " TOP ]";
        else if (this->isBottom()) ss << " BOTTOM ]";
        else {
            for (auto&& it : this->ptsTo_) {
                ss << std::endl << it->toString();
            }
            ss << std::endl << "]";
        }
        return ss.str();
    }

    Ptr load(Type::Ptr, Ptr offset) const override {
        if (this->isTop()) {
            return factory_->top(pointedType_);
        } else if (this->isBottom()) {
            return factory_->bottom(pointedType_);
        } else {
            auto&& result = factory_->bottom(pointedType_);
            for (auto&& loc : this->ptsTo_) {
                result->joinWith(loc->load(offset, pointedType_));
            }
            return result;
        }
    }

    void store(Ptr value, Ptr offset) override {
        if (this->isTop()) {
            return;
        } else if (this->isBottom()) {
            return;
        } else {
            for (auto&& loc : this->ptsTo_) {
                loc->store(offset, value);
            }
        }
    }

    Ptr gep(Type::Ptr targetType, const std::vector<ConstPtr>& offsets) const override {
        if (this->isTop()) {
            return Self::top(targetType);
        } else if (this->isBottom()) {
            return Self::bottom(targetType);
        } else {
            auto result = factory_->bottom(targetType);
            for (auto&& it : ptsTo_) {
                result->joinWith(it->gep(targetType, offsets));
            }
            return result;
        }
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_POINTER_HPP
