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
class Pointer : public AbstractDomain<Pointer<MachineInt>> {
public:

    using Ptr = std::shared_ptr<Pointer<MachineInt>>;
    using ConstPtr = std::shared_ptr<const Pointer<MachineInt>>;

    using IntervalT = Interval<MachineInt>;
    using IntervalPtr = typename IntervalT::Ptr;
    using Location = MemoryLocation<MachineInt>;
    using LocationPtr = typename Location::Ptr;
    using PointsToSet = std::unordered_set<LocationPtr>;

private:

    bool isBottom_;
    PointsToSet ptsTo_;

private:
    struct TopTag{};
    struct BottomTag{};

    explicit Pointer(TopTag) : isBottom_(false) {}
    explicit Pointer(BottomTag) : isBottom_(true) {}

public:

    Pointer() : Pointer(TopTag{}) {}
    Pointer(const Pointer&) = default;
    Pointer(Pointer&&) = default;
    Pointer& operator=(const Pointer&) = default;
    Pointer& operator=(Pointer&&) = default;
    ~Pointer() override = default;

    bool isTop() const override { return (not this->isBottom_) and this->ptsTo_.empty(); }
    bool isBottom() const override { return this->isBottom_ and this->ptsTo_.empty(); }

    bool isNull() const { return this->ptsTo_.size() == 1 and this->ptsTo_.begin()->isNull(); }
    bool ptsToNull() const {
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
        if (this->isTop()) {
            return other->isTop();
        } else if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isTop() || other->isBottom()) {
            return false;
        } else if (this->isNull()) {
            return other->isNull();
        } else {

            if (this->ptsTo_.size() != other->ptsTo_.size()) return false;

            for (auto&& it : other->ptsTo_) {
                auto&& its = this->ptsTo_.find(it);
                if (its == this->ptsTo_.end()) return false;
                if (not util::equal_with_find(it->offsets(), its->offsets(), LAM(a, a), LAM2(a, b, a->equals(b))))
                    return false;
            }
            return true;
        }
    }

    void joinWith(ConstPtr other) override {
        if (this->isBottom()) {
            this->operator=(*other.get());
        } else if (this->isTop()) {
            return;
        } else if (other->isBottom()) {
            return;
        } else if (other->isTop()) {
            this->setTop();
        } else {

            for (auto&& it : other->ptsTo_) {
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
        if (this->isBottom()) {
            return;
        } else if (this->isTop()) {
            this->operator=(*other.get());
        } else if (other->isBottom()) {
            this->setBottom();
        } else if (other->isTop()) {
            return;
        } else {

            for (auto&& it : this->ptsTo_) {
                if (not util::contains(other->ptsTo_, it)) {
                    this->ptsTo_.erase(it);
                }
            }

            for (auto&& it : other->ptsTo_) {
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

    std::string toString() const {
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

};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_POINTER_HPP
