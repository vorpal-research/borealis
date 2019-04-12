//
// Created by abdullin on 3/25/19.
//

#ifndef BOREALIS_LINEAREXPRESSION_HPP
#define BOREALIS_LINEAREXPRESSION_HPP

#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
class LinearExpression {
public:

    using VariableMap = std::unordered_map<Variable, Number, VarHash, VarEquals>;
    using VariableSet = std::unordered_set<Variable, VarHash, VarEquals>;
    using CasterT = util::Adapter<Number>;

private:

    const CasterT* caster_;
    VariableMap vars_;
    Number constant_;

public:
    explicit LinearExpression(const CasterT* caster) : LinearExpression(caster, 0) {}
    LinearExpression(const CasterT* caster, Number n) : caster_(caster), vars_{}, constant_(n) {}
    LinearExpression(const CasterT* caster, int n) : caster_(caster), vars_{}, constant_((*caster_)(n)) {}
    LinearExpression(const CasterT* caster, Variable var) : caster_(caster), vars_{}, constant_((*caster_)(0)) {
        vars_.emplace(var, (*caster_)(1));
    }

    LinearExpression(CasterT* caster, Number cst, Variable var) : caster_(caster) {
        if (cst != (*caster_)(0)) {
            this->_map.emplace(var, std::move(cst));
        }
    }

    LinearExpression(CasterT* caster, int cst, Variable var) : caster_(caster) {
        if (cst != (*caster_)(0)) {
            this->_map.emplace(var, (*caster_)(cst));
        }
    }

    LinearExpression(const LinearExpression&) = default;
    LinearExpression(LinearExpression&&) = default;
    LinearExpression& operator=(const LinearExpression&) = default;
    LinearExpression& operator=(LinearExpression&&) = default;
    ~LinearExpression() = default;

private:
    /// \brief Private constructor
    LinearExpression(CasterT* caster, VariableMap map, Number cst)
            : caster_(caster), vars_(map), constant_(cst) {}

public:
    void add(const Number& n) { constant_ += n; }
    void add(int n) { constant_ += (*caster_)(n); }

    void add(Variable var) { add((*caster_)(1), var); }

    void add(int cst, Variable var) {
        add((*caster_)(cst), var);
    }

    void add(const Number& cst, Variable var) {
        auto it = this->vars_.find(var);
        if (it != this->vars_.end()) {
            Number r = it->second + cst;
            if (r == (*caster_)(0)) {
                this->vars_.erase(it);
            } else {
                it->second = r;
            }
        } else {
            if (cst != (*caster_)(0)) {
                this->vars_.emplace(var, cst);
            }
        }
    }

    std::size_t numTerms() const { return this->vars_.size(); }

    bool isConstant() const { return this->vars_.empty(); }

    const Number& constant() const { return this->constant_; }
    const VariableSet& variables() const {
        return util::viewContainer(vars_).map(LAM(a, a.first)).template toHashSet<VarHash, VarEquals>();
    }

    Number factor(Variable var) const {
        auto it = this->vars_.find(var);
        if (it != this->vars_.end()) {
            return it->second;
        } else {
            return (*caster_)(0);
        }
    }

    const CasterT* caster() const {
        return caster_;
    }

    void operator+=(const Number& n) { this->constant_ += n; }
    void operator+=(int n) { this->constant_ += (*caster_)(n); }

    void operator+=(Variable var) { this->add(var); }

    void operator+=(const LinearExpression& expr) {
        for (const auto& term : expr) {
            this->add(term.second, term.first);
        }
        this->constant_ += expr.constant();
    }

    void operator-=(const Number& n) { this->constant_ -= n; }

    void operator-=(int n) { this->constant_ -= (*caster_)(n); }

    void operator-=(Variable var) { this->add((*caster_)(-1), var); }

    void operator-=(const LinearExpression& expr) {
        for (const auto& term : expr) {
            this->add(-term.second, term.first);
        }
        this->constant_ -= expr.constant();
    }

    LinearExpression operator-() const {
        LinearExpression r(*this);
        r *= -1;
        return r;
    }

    void operator*=(const Number& n) {
        if (n == 0) {
            this->vars_.clear();
            this->constant_ = (*caster_)(0);
        } else {
            for (auto& term : vars_) {
                term.second *= n;
            }
            this->constant_ *= n;
        }
    }

    void operator*=(int n) {
        this->operator*=((*caster_)(n));
    }

    auto begin() QUICK_RETURN(vars_.begin());
    auto begin() QUICK_CONST_RETURN(vars_.begin());

    auto end() QUICK_RETURN(vars_.end());
    auto end() QUICK_CONST_RETURN(vars_.end());

    std::string toString() const {
        std::stringstream ss;

        if (not vars_.empty()) {
            auto&& head = util::head(vars_);
            if (head.second == (*caster_)(-1)) {
                ss << "-";
            } else if (head.second != (*caster_)(0)) {
                ss << head.second;
            }
            ss << util::toString(*head.first);
        }

        for (auto&& it : util::viewContainer(vars_).drop(1)) {
            auto&& var = it.first;
            auto&& constant = it.second;

            if (constant > (*caster_)(0)) {
                ss << "+";
            }
            if (constant < (*caster_)(0)) {
                ss << "-";
            }
            if (constant != (*caster_)(1) && constant != (*caster_)(-1)) {
                ss << constant;
            }
            ss << util::toString(*var);
        }

        if (constant_ > (*caster_)(0) && not vars_.empty()) {
            ss << "+";
        }
        if (constant_ != (*caster_)(0) || vars_.empty()) {
            ss << constant_;
        }

        return ss.str();
    }
};

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
std::ostream& operator<<(std::ostream& s, const LinearExpression<Number, Variable, VarHash, VarEquals>& expr) {
    s << expr.toString();
    return s;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
borealis::logging::logstream& operator<<(borealis::logging::logstream& s, const LinearExpression<Number, Variable, VarHash, VarEquals>& expr) {
    s << expr.toString();
    return s;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator*(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    e *= n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator*(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    e *= n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator*(
        const Number& n, LinearExpression<Number, Variable, VarHash, VarEquals> e) {
    e *= n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator*(
        int n, LinearExpression<Number, Variable, VarHash, VarEquals> e) {
    e *= n;
    return e;
}

///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator+(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    e += n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator+(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    e += n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator+(
        const Number& n, LinearExpression<Number, Variable, VarHash, VarEquals> e) {
    e += n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator+(
        int n, LinearExpression<Number, Variable, VarHash, VarEquals> e) {
    e += n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator+(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    x += y;
    return x;
}

///////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator-(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, const Number& n) {
    e -= n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator-(
        LinearExpression<Number, Variable, VarHash, VarEquals> e, int n) {
    e -= n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator-(
        const Number& n, LinearExpression<Number, Variable, VarHash, VarEquals> e) {
    e *= -1;
    e += n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator-(
        int n, LinearExpression<Number, Variable, VarHash, VarEquals> e) {
    e *= -1;
    e += n;
    return e;
}

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
inline LinearExpression<Number, Variable, VarHash, VarEquals> operator-(
        LinearExpression<Number, Variable, VarHash, VarEquals> x,
        const LinearExpression<Number, Variable, VarHash, VarEquals>& y) {
    x -= y;
    return x;
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_LINEAREXPRESSION_HPP
