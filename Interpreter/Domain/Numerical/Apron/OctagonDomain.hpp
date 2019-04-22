//
// Created by abdullin on 3/28/19.
//

#ifndef BOREALIS_OCTAGONDOMAIN_HPP
#define BOREALIS_OCTAGONDOMAIN_HPP

#include <unordered_map>

#include "DoubleOctagon.hpp"
#include "Interpreter/Domain/AbstractFactory.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename N1, typename N2, typename Variable, typename VarHash, typename VarEquals>
class OctagonDomain : public NumericalDomain<Variable> {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;
    using DOctagon = DoubleOctagon<N1, N2, Variable, VarHash, VarEquals>;
    using OctagonMap = std::unordered_map<size_t, Ptr>;
    using Self = OctagonDomain<N1, N2, Variable, VarHash, VarEquals>;

protected:

    mutable OctagonMap octagons_;
    bool isBottom_;

private:

    Self* unwrap(Ptr other) const {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

    const Self* unwrap(ConstPtr other) const {
        auto* otherRaw = llvm::dyn_cast<Self>(other.get());
        ASSERTC(otherRaw);

        return otherRaw;
    }

protected:

    virtual size_t unwrapTypeSize(Variable x) const = 0;

    DOctagon* unwrapOctagon(size_t bitsize) const {
        auto&& opt = util::at(octagons_, bitsize);

        AbstractDomain::Ptr octagon;
        if (opt) {
            octagon = opt.getUnsafe();
        } else {
            octagon = DOctagon::top(util::Adapter<N1>::get(bitsize), util::Adapter<N2>::get(bitsize));
            octagons_[bitsize] = octagon;
        }

        auto* octagonRaw = llvm::dyn_cast<DOctagon>(octagon.get());
        ASSERTC(octagonRaw);
        return octagonRaw;
    }

public:
    struct TopTag{};
    struct BottomTag{};

    explicit OctagonDomain(TopTag) : NumericalDomain<Variable>(class_tag(*this)), isBottom_(false) {}
    explicit OctagonDomain(BottomTag) : NumericalDomain<Variable>(class_tag(*this)), isBottom_(true) {}

    OctagonDomain() : OctagonDomain(BottomTag{}) {}

    OctagonDomain(const OctagonDomain&) = default;
    OctagonDomain(OctagonDomain&&) = default;
    OctagonDomain& operator=(OctagonDomain&&) = default;
    ~OctagonDomain() override = default;

    OctagonDomain& operator=(const OctagonDomain& other) {
        if (this != &other) {
            this->octagons_ = other.octagons_;
            this->isBottom_ = other.isBottom_;
        }
        return *this;
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override { return not this->isBottom_ && this->octagons_.empty(); }
    bool isBottom() const override { return this->isBottom_; }

    void setTop() override { UNREACHABLE("TODO"); }
    void setBottom() override { UNREACHABLE("TODO"); }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            if (this->octagons_.size() != otherRaw->octagons_.size()) return false;

            for (auto&& it : this->octagons_) {
                auto&& otherIt = otherRaw->octagons_.find(it.first);
                if (otherIt == otherRaw->octagons_.end()) return true;
                else if (not it.second->leq((*otherIt).second)) return false;
            }
            return true;
        }
    }

    virtual bool equals(ConstPtr other) const override {
        auto* otherRaw = llvm::dyn_cast<const Self>(other.get());
        if (not otherRaw) return false;

        if (this->isBottom()) {
            return otherRaw->isBottom();
        } else if (otherRaw->isBottom()) {
            return false;
        } else {
            if (this->octagons_.size() != otherRaw->octagons_.size()) return false;

            for (auto&& it : this->octagons_) {
                auto&& otherIt = otherRaw->octagons_.find(it.first);
                if (otherIt == otherRaw->octagons_.end()) return false;
                else if (not it.second->equals((*otherIt).second)) return false;
            }
            return true;
        }
    }

    void joinWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->octagons_) {
                auto&& cur = octagons_.find(it.first);
                if (cur == octagons_.end()) octagons_[it.first] = it.second;
                else octagons_[it.first] = cur->second->join(it.second);
            }
            return;
        }
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (otherRaw->isBottom()) {
            this->setBottom();
        } else {
            for (auto&& it : otherRaw->octagons_) {
                auto&& cur = octagons_.find(it.first);
                if (cur == octagons_.end()) octagons_[it.first] = it.second;
                else octagons_[it.first] = cur->second->meet(it.second);
            }
            return;
        }
    }

    void widenWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (otherRaw->isBottom()) {
            return;
        } else if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            for (auto&& it : otherRaw->octagons_) {
                auto&& cur = octagons_.find(it.first);
                if (cur == octagons_.end()) octagons_[it.first] = it.second;
                else octagons_[it.first] = cur->second->widen(it.second);
            }
            return;
        }
    }

    Ptr join(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->joinWith(other);
        return result;
    }

    Ptr meet(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->meetWith(other);
        return result;
    }

    Ptr widen(ConstPtr other) const override {
        auto&& result = this->clone();
        auto* resultRaw = unwrap(result);
        resultRaw->widenWith(other);
        return result;
    }

    Ptr toInterval(Variable x) const override { return this->get(x); }

    bool contains(Variable x) const override {
        auto bitsize = unwrapTypeSize(x);
        auto* octagon = unwrapOctagon(bitsize);
        return octagon->contains(x);
    }

    void set(Variable x, Ptr value) {
        auto bitsize = unwrapTypeSize(x);
        auto* octagon = unwrapOctagon(bitsize);
        octagon->assign(x, value);
        this->isBottom_ = false;
    }

    void assign(Variable x, Variable y) override {
        auto bitsize = unwrapTypeSize(x);
        auto* octagon = unwrapOctagon(bitsize);
        octagon->assign(x, y);
        this->isBottom_ = false;
    }

    void assign(Variable x, Ptr i) override {
        set(x, i);
    }

    virtual void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        auto bitsize = unwrapTypeSize(x);
        auto* octagon = unwrapOctagon(bitsize);
        octagon->applyTo(op, x, y, z);
    }

    virtual Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        auto bitsize = unwrapTypeSize(x);
        auto* octagon = unwrapOctagon(bitsize);
        return octagon->applyTo(op, x, y);
    }

    virtual void addConstraint(llvm::ConditionType op, Variable x, Variable y) override {
        auto bitsize = unwrapTypeSize(x);
        auto* octagon = unwrapOctagon(bitsize);
        octagon->addConstraint(op, x, y);
    }

    size_t hashCode() const override {
        return class_tag(*this);
    }

    std::string toString() const override {
        std::stringstream ss;
        for (auto&& it : octagons_) {
            ss << "Bitsize " << it.first << std::endl;
            ss << it.second->toString();
        }
        return ss.str();
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_OCTAGONDOMAIN_HPP
