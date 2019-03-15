//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_POINTER_HPP
#define BOREALIS_POINTER_HPP

#include "Interpreter/Domain/AbstractFactory.hpp"
#include "MemoryLocation.hpp"
#include "Type/TypeUtils.h"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt>
class PointerDomain : public AbstractDomain {
public:

    using Self = PointerDomain<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using MemoryLocationT = MemoryLocation<MachineInt>;
    using PointsToSet = std::unordered_set<Ptr, AbstractDomainHash, AbstractDomainEquals>;

private:

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
    explicit PointerDomain(TopTag) :
            AbstractDomain(class_tag(*this)), isBottom_(false), factory_(AbstractFactory::get()) {}
    explicit PointerDomain(BottomTag) :
            AbstractDomain(class_tag(*this)), isBottom_(true), factory_(AbstractFactory::get()) {}


    PointerDomain() : PointerDomain(TopTag{}) {}
    explicit PointerDomain(Ptr ptsTo) :
            AbstractDomain(class_tag(*this)), isBottom_(false), factory_(AbstractFactory::get()), ptsTo_({ ptsTo }) {}
    explicit PointerDomain(const PointsToSet& ptsTo) :
            AbstractDomain(class_tag(*this)), isBottom_(false), factory_(AbstractFactory::get()), ptsTo_(ptsTo) {}
    PointerDomain(const PointerDomain&) = default;
    PointerDomain(PointerDomain&&) = default;
    PointerDomain& operator=(PointerDomain&&) = default;
    PointerDomain& operator=(const PointerDomain& other) {
        if (this != &other) {
            this->isBottom_ = other.isBottom_;
            this->factory_ = other.factory_;
            this->ptsTo_ = other.ptsTo_;
        }
        return *this;
    }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    ~PointerDomain() override = default;

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    static Ptr top() { return std::make_shared<Self>(TopTag{}); }
    static Ptr bottom() { return std::make_shared<Self>(BottomTag{}); }

    bool isTop() const override { return (not this->isBottom_) and this->ptsTo_.empty(); }
    bool isBottom() const override { return this->isBottom_ and this->ptsTo_.empty(); }

    const PointsToSet& locations() const {
        return ptsTo_;
    }

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
        if (this == other.get()) return true;

        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

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

                auto&& thisOffsets = thisLoc->offsets();
                auto&& otherOffsets = otherLoc->offsets();

                if (thisOffsets.size() != otherOffsets.size()) return false;

                for (auto&& offset : thisOffsets) {
                    auto&& otherOffset = otherOffsets.find(offset);
                    if (otherOffset == otherOffsets.end()) return false;
                    else if (not offset->equals(*otherOffset)) return false;
                }

            }
            return true;
        }
    }

    void joinWith(ConstPtr other) {
        if (this == other.get()) return;

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
                    auto&& result = (*its)->join(it);
                    this->ptsTo_.erase(its);
                    this->ptsTo_.insert(result);
                }
            }
        }
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void widenWith(ConstPtr other) {
        if (this == other.get()) return;

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
                    auto&& result = (*its)->widen(it);
                    this->ptsTo_.erase(its);
                    this->ptsTo_.insert(result);
                }
            }
        }
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    void meetWith(ConstPtr other) {
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
                    auto&& result = (*its)->meet(it);
                    this->ptsTo_.erase(its);
                    this->ptsTo_.insert(result);
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
        return class_tag(*this); //return util::hash::defaultHasher()(this->isBottom_);
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

    Ptr load(Type::Ptr type, Ptr offset) const override {
        if (this->isTop()) {
            return factory_->top(type);
        } else if (this->isBottom()) {
            return factory_->bottom(type);
        } else {
            auto&& result = factory_->bottom(type);

            for (auto&& loc : this->ptsTo_) {
                auto&& locationLoad = loc->load(type, offset);
                result = result->join(locationLoad);
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
                loc->store(value, offset);
            }
        }
    }

    Ptr gep(Type::Ptr targetType, const std::vector<Ptr>& offsets) override {
        if (this->isTop()) {
            return Self::top();
        } else if (this->isBottom()) {
            return Self::bottom();
        } else {
            auto result = factory_->bottom(targetType);
            for (auto&& it : ptsTo_) {
                result = result->join(it->gep(targetType, offsets));
            }
            return result;
        }
    }

    Split splitByEq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isNull() || this->isTop() || this->isBottom() || other->isTop() || other->isBottom())
            return { clone(), clone() };
        else if (this->isTop() && otherRaw->isNull()) {
            // This is generally fucked up
            auto&& arrayType = factory_->tf()->getArray(factory_->tf()->getBool(), 1);
            return {
                factory_->getNullptr(),
                factory_->getPointer(factory_->getArray(arrayType, AbstractFactory::TOP), factory_->getMachineInt(AbstractFactory::TOP))
            };
        }

        PointsToSet trueLocs, falseLocs;
        for (auto&& loc : otherRaw->ptsTo_) {
            if (util::contains(this->ptsTo_, loc)) {
                trueLocs.insert(loc);
            } else {
                falseLocs.insert(loc);
            }
        }
        return { std::make_shared<Self>(trueLocs), std::make_shared<Self>(falseLocs) };
    }

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_POINTER_HPP
