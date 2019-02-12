//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_POINTER_HPP
#define BOREALIS_POINTER_HPP

#include "Interpreter/Domain/AbstractDomain.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"
#include "MemoryLocation.hpp"
#include "Type/TypeUtils.h"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt>
class Pointer : public AbstractDomain {
public:

    using Self = Pointer<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using MemoryLocationT = MemoryLocation<MachineInt>;
    using PointsToSet = std::unordered_set<Ptr>;

private:

    Type::Ptr pointedType_;
    bool isBottom_;
    AbstractFactory* factory_;
    PointsToSet ptsTo_;

private:
    struct TopTag{};
    struct BottomTag{};

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

    MemoryLocationT* unwrapLocation(Ptr loc) const {
        auto* ptr = llvm::dyn_cast<MemoryLocationT>(loc.get());
        ASSERTC(ptr);
        return ptr;
    }

public:
    Pointer(TopTag, Type::Ptr type) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(false), factory_(AbstractFactory::get()) {}
    Pointer(BottomTag, Type::Ptr type) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(true), factory_(AbstractFactory::get()) {}


    explicit Pointer(Type::Ptr type) : Pointer(TopTag{}, type) {}
    Pointer(Type::Ptr type, Ptr ptsTo) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(false), factory_(AbstractFactory::get()), ptsTo_({ ptsTo }) {}
    Pointer(Type::Ptr type, const PointsToSet& ptsTo) :
            AbstractDomain(class_tag(*this)), pointedType_(type), isBottom_(false), factory_(AbstractFactory::get()), ptsTo_(ptsTo) {}
    Pointer(const Pointer&) = default;
    Pointer(Pointer&&) = default;
    Pointer& operator=(Pointer&&) = default;
    Pointer& operator=(const Pointer& other) {
        if (this != &other) {
            this->pointedType_ = other.pointedType_;
            this->isBottom_ = other.isBottom_;
            this->factory_ = other.factory_;
            this->ptsTo_ = other.ptsTo_;
        }
        return *this;
    }

    ~Pointer() override = default;

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    static Ptr top(Type::Ptr type) { return std::make_shared<Self>(TopTag{}, type); }
    static Ptr bottom(Type::Ptr type) { return std::make_shared<Self>(BottomTag{}, type); }

    bool isTop() const override { return (not this->isBottom_) and this->ptsTo_.empty(); }
    bool isBottom() const override { return this->isBottom_ and this->ptsTo_.empty(); }

    bool isNull() const { return this->ptsTo_.size() == 1 and unwrapLocation(util::head(this->ptsTo_))->isNull(); }
    bool pointsToNull() const {
        for (auto&& it : this->ptsTo_) {
            if (unwrapLocation(it)->isNull()) return true;
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

                auto* thisLoc = unwrapLocation(its.operator*());
                auto* otherLoc = unwrapLocation(it);

                if (not util::equal_with_find(otherLoc->offsets(), thisLoc->offsets(), LAM(a, a), LAM2(a, b, a->equals(b))))
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
                    (*its)->joinWith(it);
                }
            }
        }
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void widenWith(ConstPtr other) override { joinWith(other); }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

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
                    (*its)->meetWith(it);
                }
            }
        }
    }

    Ptr meet(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->meetWith(other);
        return next;
    }

    size_t hashCode() const override {
        return util::hash::defaultHasher()(this->isBottom_, this->ptsTo_.size());
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
                result->joinWith(loc->load(pointedType_, offset));
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

    Ptr gep(Type::Ptr targetType, const std::vector<Ptr>& offsets) override {
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
