//
// Created by abdullin on 2/4/19.
//

#ifndef BOREALIS_ARRAYDOMAIN_HPP
#define BOREALIS_ARRAYDOMAIN_HPP

#include <unordered_map>

#include "Interpreter/Domain/DomainFactory.h"
#include "Pointer.hpp"

#include "Util/collections.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename MachineInt>
class ArrayDomain : public AbstractDomain {
public:

    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using Self = ArrayDomain<MachineInt>;
    using IntervalT = Interval<MachineInt>;
    using ElementMapT = std::unordered_map<size_t, AbstractDomain::Ptr>;

private:

    Ptr length_;
    Type::Ptr elementType_;
    AbstractFactory* factory_;
    ElementMapT elements_;

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

    IntervalT* length() const {
        auto* len = llvm::dyn_cast<IntervalT>(length_.get());
        ASSERTC(len);
        return len;
    }

public:
    struct TopTag {};
    struct BottomTag {};

    ArrayDomain(TopTag, Type::Ptr elementType) :
            AbstractDomain(class_tag(*this)), length_(IntervalT::top()), elementType_(elementType), factory_(AbstractFactory::get()) {}

    ArrayDomain(BottomTag, Type::Ptr elementType) :
            AbstractDomain(class_tag(*this)), length_(IntervalT::bottom()), elementType_(elementType), factory_(AbstractFactory::get()) {}


    explicit ArrayDomain(Type::Ptr elementType) : ArrayDomain(TopTag{}, elementType) {}
    ArrayDomain(Type::Ptr elementType, const std::vector<AbstractDomain::Ptr>& elements) :
            AbstractDomain(class_tag(*this)), length_(IntervalT::constant((int) elements.size())), elementType_(elementType), factory_(AbstractFactory::get()) {
        for (auto i = 0U; i < elements.size(); ++i) {
            elements_[i] = elements[i];
        }
    }

    ArrayDomain(Type::Ptr elementType, Ptr length) :
            AbstractDomain(class_tag(*this)), length_(length), elementType_(elementType), factory_(AbstractFactory::get()) {}

    ArrayDomain(const ArrayDomain&) = default;
    ArrayDomain(ArrayDomain&&) = default;
    ArrayDomain& operator=(ArrayDomain&&) = default;
    ArrayDomain& operator=(const ArrayDomain& other) {
        if (this != &other) {
            this->length_ = other.length_;
            this->elementType_ = other.elementType_;
            this->factory_ = other.factory_;
            this->elements_ = other.elements_;
        }
        return *this;
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    static Ptr top(Type::Ptr elementType) { return std::make_shared<ArrayDomain>(TopTag{}, elementType); }
    static Ptr bottom(Type::Ptr elementType) { return std::make_shared<ArrayDomain>(BottomTag{}, elementType); }
    static Ptr value(Type::Ptr elementType, Ptr length) { return std::make_shared<ArrayDomain>(elementType, length); }
    static Ptr constant(Type::Ptr elementType, const std::vector<AbstractDomain::Ptr>& elements) {
        return std::make_shared<ArrayDomain>(elementType, elements);
    }

    Type::Ptr elementType() const { return elementType_; }

    bool isTop() const override { return length_->isTop() and elements_.empty(); }
    bool isBottom() const override { return length_->isBottom() and elements_.empty(); }

    void setTop() override {
        length_->setTop();
        elements_.clear();
    }

    void setBottom() override {
        length_->setBottom();
        elements_.clear();
    }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else {
            return this->length_ < otherRaw->length_ and
            util::equal_with_find(this->elements_, otherRaw->elements_, LAM(a, a.first), LAM2(a, b, a.second->leq(b.second)));
        }
    }

    bool equals(ConstPtr other) const  override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->isBottom();
        } else if (this->isTop()) {
            return other->isTop();
        } else if (other->isTop() || other->isBottom()) {
            return false;
        } else {

            if (this->length_ != otherRaw->length_) return false;

            for (auto&& it : this->elements_) {
                auto&& opt = util::at(otherRaw->elements_, (size_t) it.first);
                if ((not opt) || (not it.second->equals(opt.getUnsafe()))) {
                    return false;
                }
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
            this->operator=(*otherRaw);
        } else {

            this->length_->joinWith(otherRaw->length_);
            for (auto&& it : otherRaw->elements_) {
                auto&& opt = util::at(this->elements_, (size_t) it.first);
                if (not opt) {
                    this->elements_[it.first] = it.second;
                } else {
                    this->elements_[it.first] = it.second->join(opt.getUnsafe());
                }
            }
        }
    }

    Ptr join(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->joinWith(other);
        return next;
    }

    void meetWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return;
        } else if (this->isTop()) {
            this->operator=(*otherRaw);
        } else if (other->isBottom()) {
            this->operator=(*otherRaw);
        } else if (other->isTop()) {
            return;
        } else {

            this->length_->meetWith(otherRaw->length_);
            for (auto&& it : otherRaw->elements_) {
                auto&& opt = util::at(this->elements_, (size_t) it.first);
                if (not opt) {
                    this->elements_[it.first] = it.second;
                } else {
                    this->elements_[it.first] = it.second->meet(opt.getUnsafe());
                }
            }
        }
    }

    Ptr widen(ConstPtr other) const override {
        auto next = std::make_shared<Self>(*this);
        next->widenWith(other);
        return next;
    }

    void widenWith(ConstPtr other) override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            this->operator=(*otherRaw);
        } else if (this->isTop()) {
            return;
        } else if (other->isBottom()) {
            return;
        } else if (other->isTop()) {
            this->operator=(*otherRaw);
        } else {

            this->length_->widenWith(otherRaw->length_);
            for (auto&& it : otherRaw->elements_) {
                auto&& opt = util::at(this->elements_, (size_t) it.first);
                if (not opt) {
                    this->elements_[it.first] = it.second;
                } else {
                    this->elements_[it.first] = it.second->widen(opt.getUnsafe());
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
        return util::hash::defaultHasher()(this->length_, this->elements_.size());
    }

    std::string toString() const override {
        std::ostringstream ss;
        ss << "Array [" << this->length()->ub() << " x " << TypeUtils::toString(*elementType_.get()) << "] ";
        ss << ": [";
        if (this->isTop()) {
            ss << " TOP ]";
        } else if (this->isBottom()) {
            ss << " BOTTOM ]";
        } else {
            for (auto&& it : this->elements_) {
                ss << std::endl << "  " << it.first << " : " << it.second->toString();
            }
            ss << std::endl << "]";
        }
        return ss.str();
    }

    Ptr load(Type::Ptr, Ptr interval) const override {
        auto* intervalRaw = llvm::dyn_cast<IntervalT>(interval.get());
        ASSERTC(intervalRaw);

        if (this->isTop()) {
            return factory_->top(elementType_);
        } else if (this->isBottom()) {
            return factory_->bottom(elementType_);
        } else {
            auto result = factory_->bottom(elementType_);

            auto lengthUb = this->length()->ub();
            auto lb = intervalRaw->lb();
            auto ub = intervalRaw->ub();

            if (ub >= lengthUb) {
                warns() << "Possible buffer overflow" << endl;
            } else if (lb >= lengthUb) {
                warns() << "Buffer overflow" << endl;
                return factory_->top(elementType_);
            }

            for (auto i = lb; i < ub and i < lengthUb; ++i) {
                auto&& opt = util::at(this->elements_, (size_t) i);
                result->joinWith((not opt) ? factory_->bottom(elementType_) : opt.getUnsafe());
            }

            return result;
        }
    }

    void store(Ptr value, Ptr interval) override {
        auto* intervalRaw = llvm::dyn_cast<IntervalT>(interval.get());
        ASSERTC(intervalRaw);

        if (this->isTop()) {
            return;
        } else if (this->isBottom()) {
            return;
        } else {
            auto lengthUb = this->length()->ub();
            auto lb = intervalRaw->lb();
            auto ub = intervalRaw->ub();

            if (ub >= lengthUb) {
                warns() << "Possible buffer overflow" << endl;
            } else if (lb >= lengthUb) {
                warns() << "Buffer overflow" << endl;
            }

            for (auto i = lb; i < ub and i < lengthUb; ++i) {
                auto&& opt = util::at(this->elements_, (size_t) i);
                if (not opt) {
                    this->elements_[(size_t) i] = value;
                } else {
                    opt.getUnsafe()->joinWith(value);
                }
            }
        }
    }

    Ptr gep(Type::Ptr type, const std::vector<Ptr>& offsets) override {
        if (this->isTop()) {
            return factory_->top(type);
        } else if (this->isBottom()) {
            return factory_->top(type);
        }

        auto* intervalRaw = llvm::dyn_cast<IntervalT>(offsets[0]);
        ASSERTC(intervalRaw);

        auto& length = this->length()->ub();
        auto& idx_begin = intervalRaw->lb();
        auto& idx_end = intervalRaw->ub();

        if (idx_end > length) {
            warns() << "Possible buffer overflow" << endl;
        } else if (idx_begin > length) {
            warns() << "Buffer overflow" << endl;
        }

        if (offsets.size() == 1) {
            return factory_->getPointer(type, shared_from_this(), offsets[0]);

        } else {
            Ptr result = factory_->bottom(type);

            std::vector<Ptr> sub_idx(offsets.begin() + 1, offsets.end());
            for (auto i = idx_begin; i <= idx_end && i < length; ++i) {
                if (not util::at(elements_, (size_t) i)) {
                    elements_[(size_t) i] = factory_->bottom(elementType_);
                }
                auto subGep = elements_[(size_t) i]->gep(type, sub_idx);
                result->joinWith(subGep);
            }
            if (not result) {
                warns() << "Gep is out of bounds" << endl;
                return factory_->top(type);
            }

            return result;
        }
    }
};

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_ARRAYDOMAIN_HPP
