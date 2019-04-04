//
// Created by abdullin on 3/22/19.
//

#ifndef BOREALIS_APRONDOMAIN_HPP
#define BOREALIS_APRONDOMAIN_HPP

#include <gmpxx.h>

#include <ap_global0.h>
#include <ap_pkgrid.h>
#include <ap_ppl.h>
#include <box.h>
#include <oct.h>
#include <pk.h>
#include <pkeq.h>
#include <ap_scalar.h>

#include "LinearExpression.hpp"
#include "LinearConstraint.hpp"
#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {
namespace apron {

enum DomainT {
    INTERVAL,
    OCTAGON
};

namespace impl_ {

using ApronPtr = std::shared_ptr<ap_abstract0_t>;

inline ap_manager_t* allocate_manager(DomainT domain) {
    switch (domain) {
        case INTERVAL:
            return ::box_manager_alloc();
        case OCTAGON:
            return ::oct_manager_alloc();
        default:
            UNREACHABLE("Unknown apron domain");
    }
}

inline ApronPtr newPtr(ap_abstract0_t* domain) {
    return ApronPtr{domain, LAM(a, ap_abstract0_free(ap_abstract0_manager(a), a))};
}

inline size_t dims(ap_abstract0_t* ptr) {
    return ap_abstract0_dimension(ap_abstract0_manager(ptr), ptr).intdim;
}

inline ApronPtr addDimensions(ap_abstract0_t* ptr, size_t dims) {
    ASSERTC(dims > 0);

    auto* dim_change = ap_dimchange_alloc(dims, 0);
    for (auto i = 0U; i < dims; ++i) {
        dim_change->dim[i] = static_cast<ap_dim_t>(impl_::dims(ptr));
    }

    auto* manager = ap_abstract0_manager(ptr);
    auto new_ptr = newPtr(ap_abstract0_add_dimensions(manager, false, ptr, dim_change, false));
    ap_dimchange_free(dim_change);

    return new_ptr;
}

inline ApronPtr removeDimensions(ap_abstract0_t* ptr, const std::vector<ap_dim_t>& dims) {
    ASSERTC(not dims.empty());
    ASSERTC(std::is_sorted(dims.begin(), dims.end()));

    auto* dimchange = ap_dimchange_alloc(dims.size(), 0);

    for (auto i = 0U; i < dims.size(); i++) {
        dimchange->dim[i] = dims[i];
    }

    auto* manager = ap_abstract0_manager(ptr);
    auto new_ptr = newPtr(ap_abstract0_remove_dimensions(manager, false, ptr, dimchange));
    ap_dimchange_free(dimchange);
    return new_ptr;
}

template <typename Number>
inline ap_texpr0_t* binopExpr(ap_texpr_op_t, ap_texpr0_t*, ap_texpr0_t*);

template <>
inline ap_texpr0_t* binopExpr<BitInt<true>>(ap_texpr_op_t op, ap_texpr0_t* l, ap_texpr0_t* r) {
    return ap_texpr0_binop(op, l, r, AP_RTYPE_INT, AP_RDIR_ZERO);
}

template <>
inline ap_texpr0_t* binopExpr<BitInt<false>>(ap_texpr_op_t op, ap_texpr0_t* l, ap_texpr0_t* r) {
    return ap_texpr0_binop(op, l, r, AP_RTYPE_INT, AP_RDIR_ZERO);
}

template <>
inline ap_texpr0_t* binopExpr<Float>(ap_texpr_op_t op, ap_texpr0_t* l, ap_texpr0_t* r) {
    return ap_texpr0_binop(op, l, r, AP_RTYPE_REAL, AP_RDIR_NEAREST);
}

template <typename Number>
inline ap_texpr0_t* toAExpr(const Number&) {
    UNREACHABLE("Unimplemented");
}

template <>
inline ap_texpr0_t* toAExpr(const BitInt<true>& n) {
    mpq_class e(n.toGMP());
    return ap_texpr0_cst_scalar_mpq(e.get_mpq_t());
}

template <>
inline ap_texpr0_t* toAExpr(const BitInt<false>& n) {
    mpq_class e(n.toGMP());
    return ap_texpr0_cst_scalar_mpq(e.get_mpq_t());
}

template <>
inline ap_texpr0_t* toAExpr(const Float& n) {
    mpq_class e(n.toGMP());
    return ap_texpr0_cst_scalar_mpq(e.get_mpq_t());
}

template <typename Number>
inline Number toBNumber(ap_scalar_t*, const util::Adapter<Number>* caster);

template <>
inline BitInt<true> toBNumber(ap_scalar_t* scalar, const util::Adapter<BitInt<true>>* caster) {
    ASSERTC(ap_scalar_infty(scalar) == 0);
//    ASSERTC(scalar->discr == AP_SCALAR_MPQ);

    auto q = mpq_class(scalar->val.dbl);

//    std::stringstream ss;
//    ss << q;

    return (*caster)(q.get_d());
}

template <>
inline BitInt<false> toBNumber(ap_scalar_t* scalar, const util::Adapter<BitInt<false>>* caster) {
    ASSERTC(ap_scalar_infty(scalar) == 0);
//    ASSERTC(scalar->discr == AP_SCALAR_MPQ);

    auto q = mpq_class(scalar->val.dbl);

//    std::stringstream ss;
//    ss << q;

    return (*caster)(q.get_d());
}

template <>
inline Float toBNumber(ap_scalar_t* scalar, const util::Adapter<Float>* caster) {
    ASSERTC(ap_scalar_infty(scalar) == 0);
    ASSERTC(scalar->discr == AP_SCALAR_MPQ);

    auto q = mpq_class(scalar->val.mpq);
    std::stringstream ss;
    ss << q;

    return (*caster)(ss.str());
}

template <typename Number>
inline Number toBNumber(ap_coeff_t* coeff, const util::Adapter<Number>* caster) {
    ASSERTC(coeff->discr == AP_COEFF_SCALAR);

    return toBNumber<Number>(coeff->val.scalar, caster);
}

template <typename Number>
inline Bound<Number> toBBound(ap_scalar_t* scalar, const util::Adapter<Number>* caster) {
    using BoundT = Bound<Number>;

    if (ap_scalar_infty(scalar) == -1) {
        return BoundT::minusInfinity(caster);
    } else if (ap_scalar_infty(scalar) == 1) {
        return BoundT::plusInfinity(caster);
    } else {
        return BoundT(caster, toBNumber<Number>(scalar, caster));
    }
}

template <typename Number>
inline typename Interval<Number>::Ptr toBInterval(ap_interval_t* intv, const util::Adapter<Number>* caster) {
    using IntervalT = Interval<Number>;

    if (ap_interval_is_top(intv)) {
        return IntervalT::top(caster);
    }

    if (ap_interval_is_bottom(intv)) {
        return IntervalT::bottom(caster);
    }

    return IntervalT::interval(toBBound<Number>(intv->inf, caster), toBBound<Number>(intv->sup, caster));
}

} // namespace impl_

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

static config::BoolConfigEntry enableOctagonPrint("absint", "enable-octagon-printing");

template <typename Number, typename Variable, typename VarHash, typename VarEquals, apron::DomainT domain>
class ApronDomain : public NumericalDomain<Variable> {
public:
    using Self = ApronDomain<Number, Variable, VarHash, VarEquals, domain>;
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using IntervalT = Interval<Number>;
    using CasterT = typename IntervalT::CasterT;
    using LinearConstSystemT = LinearConstraintSystem<Number, Variable, VarHash, VarEquals>;
    using LinearConstraintT = typename LinearConstSystemT::LinearConstraintT;
    using LinearExprT = typename LinearConstSystemT::LinearExpr;

private:

    using VariableMap = std::unordered_map<Variable, size_t, VarHash, VarEquals>;

private:

    const CasterT* caster_;
    VariableMap vars_;
    impl_::ApronPtr env_;

private:

    static ap_manager_t* manager() {
        static auto* m = impl_::allocate_manager(domain);
        return m;
    }

    util::option_ref<const size_t> getVarDim(Variable x) const {
        return util::at(vars_, x);
    }

    size_t getOrPutVar(Variable x) {
        auto&& opt = getVarDim(x);
        if (opt) return opt.getUnsafe();

        auto num = vars_.size();
        env_ = impl_::addDimensions(env_.get(), 1);
        vars_.insert({ x, num });

        ASSERTC(vars_.size() == impl_::dims(env_.get()));
        return num;
    }

    static ap_dimperm_t* build_perm_map(const VariableMap& old_map,
                                        const VariableMap& new_map) {
        std::size_t n = new_map.size();
        ap_dimperm_t* perm = ap_dimperm_alloc(n);
        std::vector<bool> index_assigned(n, false);
        std::vector<bool> value_assigned(n, false);

        for (auto&& it : old_map) {
            auto&& opt = util::at(new_map, it.first);
            ASSERTC(opt);
            auto dim = opt.getUnsafe();

            perm->dim[it.second] = dim;
            index_assigned[it.second] = true;
            value_assigned[dim] = true;
        }

        ap_dim_t counter = 0;
        for (ap_dim_t i = 0; i < n; i++) {
            if (index_assigned[i]) {
                continue;
            }

            while (value_assigned[counter]) {
                counter++;
            }

            perm->dim[i] = counter;
            counter++;
        }

        return perm;
    }

    static VariableMap merge_var_maps(const VariableMap& var_map_x,
                                      impl_::ApronPtr& inv_x,
                                      const VariableMap& var_map_y,
                                      impl_::ApronPtr& inv_y) {
        ASSERTC(var_map_x.size() == impl_::dims(inv_x.get()));
        ASSERTC(var_map_y.size() == impl_::dims(inv_y.get()));

        VariableMap result_var_map(var_map_x);
        bool equal = (var_map_x.size() == var_map_y.size());

        for (auto&& it : var_map_y) {
            auto&& v_y = it.first;
            auto&& dim_y = it.second;
            auto&& dim_x = util::at(var_map_x, v_y);
            equal = equal && dim_x && dim_x.getUnsafe() == dim_y;

            if (not dim_x) {
                auto dim = result_var_map.size();
                result_var_map.insert({ v_y, dim });
            }
        }

        if (equal) {
            return result_var_map;
        }

        if (result_var_map.size() > var_map_x.size()) {
            inv_x = impl_::addDimensions(inv_x.get(), result_var_map.size() - var_map_x.size());
        }
        if (result_var_map.size() > var_map_y.size()) {
            inv_y = impl_::addDimensions(inv_y.get(), result_var_map.size() - var_map_y.size());
        }

        ASSERTC(result_var_map.size() == impl_::dims(inv_x.get()));
        ASSERTC(result_var_map.size() == impl_::dims(inv_y.get()));

        auto* perm_y = build_perm_map(var_map_y, result_var_map);
        inv_y = impl_::newPtr(ap_abstract0_permute_dimensions(manager(), false, inv_y.get(), perm_y));
        ap_dimperm_free(perm_y);

        ASSERTC(result_var_map.size() == impl_::dims(inv_x.get()));
        ASSERTC(result_var_map.size() == impl_::dims(inv_y.get()));

        return result_var_map;
    }

    ap_texpr0_t* toAExpr(Variable x) {
        return ap_texpr0_dim(getOrPutVar(x));
    }

    ap_texpr0_t* toAExpr(const LinearExprT& le) {
        auto* r = impl_::toAExpr(le.constant());

        for (auto&& it : le) {
            auto* term = impl_::binopExpr<Number>(AP_TEXPR_MUL, impl_::toAExpr(it.second), toAExpr(it.first));
            r = impl_::binopExpr<Number>(AP_TEXPR_ADD, r, term);
        }

        return r;
    }

    ap_tcons0_t toAConstraint(const LinearConstraintT& cst) {
        const LinearExprT& exp = cst.expression();

        if (cst.isEquality()) {
            return ap_tcons0_make(AP_CONS_EQ, toAExpr(exp), nullptr);
        } else if (cst.isInequality()) {
            return ap_tcons0_make(AP_CONS_SUPEQ, toAExpr(-exp), nullptr);
        } else {
            return ap_tcons0_make(AP_CONS_DISEQ, toAExpr(exp), nullptr);
        }
    }

    LinearExprT toLinearExpr(ap_linexpr0_t* expr) const {
        ASSERTC(ap_linexpr0_is_linear(expr));

        ap_coeff_t* coeff = ap_linexpr0_cstref(expr);
        LinearExprT e(caster_, impl_::toBNumber<Number>(coeff, caster_));

        for (auto it = vars_.begin(), et = vars_.end(); it != et; ++it) {
            coeff = ap_linexpr0_coeffref(expr, it->second);

            if (ap_coeff_zero(coeff)) {
                continue;
            }

            e.add(impl_::toBNumber<Number>(coeff, caster_), it->first);
        }

        return e;
    }

    LinearConstraintT toLinearConstraint(const ap_lincons0_t& cst) const {
        ASSERTC(cst.scalar == nullptr);

        LinearExprT e = toLinearExpr(cst.linexpr0);

        switch (cst.constyp) {
            case AP_CONS_EQ:
                return LinearConstraintT(std::move(e), LinearConstraintT::EQUALITY);
            case AP_CONS_SUPEQ:
                return LinearConstraintT(-std::move(e), LinearConstraintT::COMPARSION);
            case AP_CONS_SUP:
                return LinearConstraintT(-std::move(e) + 1,
                                         LinearConstraintT::COMPARSION);
            case AP_CONS_DISEQ:
                return LinearConstraintT(std::move(e), LinearConstraintT::INEQUALITY);
            case AP_CONS_EQMOD:
            default:
                UNREACHABLE("unexpected linear constraint");
        }
    }

    bool isSupported(llvm::ArithType op) const {
        return false;
        switch (op) {
            case llvm::ArithType::ADD:
            case llvm::ArithType::SUB:
            case llvm::ArithType::MUL:
            case llvm::ArithType::DIV:
            case llvm::ArithType::REM:
                return true;
            default:
                return false;
        }
    }

    void applyToAExpr(llvm::ArithType op, Variable x, ap_texpr0_t* lhv, ap_texpr0_t* rhv) {
        ap_texpr0_t* t;

        switch (op) {
            case llvm::ArithType::ADD: {
                t = impl_::binopExpr<Number>(AP_TEXPR_ADD, lhv, rhv);
            } break;
            case llvm::ArithType::SUB: {
                t = impl_::binopExpr<Number>(AP_TEXPR_SUB, lhv, rhv);
            } break;
            case llvm::ArithType::MUL: {
                t = impl_::binopExpr<Number>(AP_TEXPR_MUL, lhv, rhv);
            } break;
            case llvm::ArithType::DIV: {
                t = impl_::binopExpr<Number>(AP_TEXPR_DIV, lhv, rhv);
            } break;
            case llvm::ArithType::REM: {
                t = impl_::binopExpr<Number>(AP_TEXPR_MOD, lhv, rhv);
            } break;
            default:
                UNREACHABLE("unsupported operator");
        }

        auto x_dim = getOrPutVar(x);
        this->env_ = impl_::newPtr(ap_abstract0_assign_texpr(manager(), false, this->env_.get(), x_dim, t, nullptr));
        ap_texpr0_free(t);
    }

private:
    struct TopTag {};
    struct BottomTag {};

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

    Ptr toInterval(const Number& n) const {
        return IntervalT::constant(n, caster_);
    }

public:
    ApronDomain(const CasterT* caster, TopTag)
            : NumericalDomain<Variable>(class_tag(*this)),
                    caster_(caster),
                    env_(impl_::newPtr(ap_abstract0_top(manager(), 0, 0))) {}

    ApronDomain(const CasterT* caster, BottomTag)
            : NumericalDomain<Variable>(class_tag(*this)),
                    caster_(caster),
                    env_(impl_::newPtr(ap_abstract0_bottom(manager(), 0, 0))) {}

    explicit ApronDomain(const CasterT* caster) : ApronDomain(caster, TopTag{}) {}
    ApronDomain(const CasterT* caster, VariableMap vars, impl_::ApronPtr env)
            : NumericalDomain<Variable>(class_tag(*this)), caster_(caster), vars_(std::move(vars)), env_(env) {}

    ApronDomain(const ApronDomain&) = default;
    ApronDomain(ApronDomain&&) = default;
    ApronDomain& operator=(ApronDomain&&) = default;
    ApronDomain& operator=(const ApronDomain& other) {
        if (this != &other) {
            this->caster_ = other.caster_;
            this->vars_ = other.vars_;
            this->env_ = other.env_;
        }
        return *this;
    }

    virtual ~ApronDomain() = default;

    static Ptr top(const CasterT* caster) { return std::make_shared<ApronDomain>(caster, TopTag{}); }
    static Ptr bottom(const CasterT* caster) { return std::make_shared<ApronDomain>(caster, BottomTag{}); }

    const CasterT* caster() const {
        return caster_;
    }

    Ptr clone() const override {
        return std::make_shared<Self>(*this);
    }

    static bool classof(const Self*) {
        return true;
    }

    static bool classof(const AbstractDomain* other) {
        return other->getClassTag() == class_tag<Self>();
    }

    bool isTop() const override {
        return ap_abstract0_is_top(manager(), env_.get());
    }

    bool isBottom() const override {
        return ap_abstract0_is_bottom(manager(), env_.get());
    }

    void setTop() override {
        this->operator=(*unwrap(top(caster_)));
    }

    void setBottom() override {
        this->operator=(*unwrap(bottom(caster_)));
    }

    bool leq(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return true;
        } else if (other->isBottom()) {
            return false;
        } else if (other->isTop()) {
            return true;
        } else if (this->isTop()) {
            return false;
        } else {
            impl_::ApronPtr inv_x(this->env_);
            impl_::ApronPtr inv_y(otherRaw->env_);
            merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            return ap_abstract0_is_leq(manager(), inv_x.get(), inv_y.get());
        }
    }

    bool equals(ConstPtr other) const override {
        auto* otherRaw = unwrap(other);

        if (this->isBottom()) {
            return other->isBottom();
        } else if (other->isBottom()) {
            return false;
        } else if (this->isTop()) {
            return other->isTop();
        } else if (other->isTop()) {
            return false;
        } else {
            impl_::ApronPtr inv_x(this->env_);
            impl_::ApronPtr inv_y(otherRaw->env_);
            merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            return ap_abstract0_is_eq(manager(), inv_x.get(), inv_y.get());
        }
    }

    void joinWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || other->isTop()) {
            this->operator=(*otherRaw);
        } else if (this->isTop() || other->isBottom()) {
            return;
        } else {
            impl_::ApronPtr inv_x(this->env_);
            impl_::ApronPtr inv_y(otherRaw->env_);
            VariableMap var_map = merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            this->env_ = impl_::newPtr(ap_abstract0_join(manager(), false, inv_x.get(), inv_y.get()));
            this->vars_ = std::move(var_map);
        }
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || other->isTop()) {
            return;
        } else if (this->isTop() || other->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            impl_::ApronPtr inv_x(this->env_);
            impl_::ApronPtr inv_y(otherRaw->env_);
            VariableMap var_map = merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            this->env_ = impl_::newPtr(ap_abstract0_meet(manager(), false, inv_x.get(), inv_y.get()));
            this->vars_ = std::move(var_map);
        }
    }

    void widenWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || other->isTop()) {
            this->operator=(*otherRaw);
        } else if (this->isTop() || other->isBottom()) {
            return;
        } else {
            impl_::ApronPtr inv_x(this->env_);
            impl_::ApronPtr inv_y(otherRaw->env_);
            VariableMap var_map = merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            this->env_ = impl_::newPtr(ap_abstract0_widening(manager(), inv_x.get(), inv_y.get()));
            this->vars_ = std::move(var_map);
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

    Ptr get(Variable x) const override {
       return toInterval(x);
    }

    void set(Variable x, Ptr value) {
        if (auto* interval = llvm::dyn_cast<IntervalT>(value.get()))
            set(x, *interval);
        else
            UNREACHABLE("Unknown numerical domain");
    }

    void set(Variable x, const IntervalT& interval) {
        if (this->isBottom()) {
            return;
        } else if (interval.isBottom()) {
            this->setBottom();
        } else {
            this->forget(x);
            this->refine(x, interval);
        }
    }

    void forget(Variable x) {
        auto&& has_dim = getVarDim(x);

        if (not has_dim) {
            return;
        }

        auto dim = has_dim.getUnsafe();
        std::vector< ap_dim_t > vector_dims{ static_cast<ap_dim_t>(dim) };
        this->env_ = impl_::newPtr(ap_abstract0_forget_array(manager(), false,
                this->env_.get(), &vector_dims[0], vector_dims.size(), false));

        this->env_ = impl_::removeDimensions(this->env_.get(), vector_dims);
        VariableMap newVars;
        for (auto&& it : vars_) {
            if (it.second < dim) newVars.insert(it);
            else if (it.second > dim) newVars.insert({ it.first, it.second - 1 });
        }
        this->vars_ = std::move(newVars);
        ASSERTC(this->vars_.size() == impl_::dims(this->env_.get()));
    }

    void refine(Variable x, const IntervalT& interval) {
        this->add(LinearConstSystemT::withinInterval(x, interval));
    }

    void add(const LinearConstraintT& cst) {
        add(LinearConstSystemT{ cst });
    }

    void add(const LinearConstSystemT& csts) {
        if (csts.empty()) {
            return;
        }

        if (this->isBottom()) {
            return;
        }

        auto ap_csts = ap_tcons0_array_make(csts.size());

        size_t i = 0;
        for (const LinearConstraintT& cst : csts) {
            ap_csts.p[i++] = toAConstraint(cst);
        }

        this->env_ = impl_::newPtr(ap_abstract0_meet_tcons_array(manager(), false, this->env_.get(), &ap_csts));
        ap_tcons0_array_clear(&ap_csts);
    }

    Ptr toInterval(Variable x) const override {
        if (this->isBottom()) {
            return IntervalT::bottom(caster_);
        }

        auto dim = getVarDim(x);

        if (dim) {
            ap_interval_t* intv = ap_abstract0_bound_dimension(manager(), this->env_.get(), dim.getUnsafe());
            auto r = impl_::toBInterval<Number>(intv, caster_);
            ap_interval_free(intv);
            return r;
        } else {
            return IntervalT::top(caster_);
        }
    }

    void assign(Variable x, Variable y) override {
        LinearExprT xe(caster_, x);
        xe -= y;
        add(xe == (*caster_)(0));
    }

    void assign(Variable x, Ptr i) override {
        set(x, i);
    }

    void assign(Variable x, const LinearExprT& expr) {
        if (this->isBottom()) {
            return;
        }

        auto* t = toAExpr(expr);
        auto vDim = getOrPutVar(x);
        this->_inv = newPtr(ap_abstract0_assign_texpr(manager(), false, this->env_.get(), vDim, t, nullptr));
        ap_texpr0_free(t);
    }

    void normalize() const {
        ap_abstract0_canonicalize(manager(), this->env_.get());
    }

    size_t hashCode() const override {
        return 0;
    }

    std::string toConstraintString() const {
        this->normalize();

        std::stringstream out;

        if (this->isBottom()) {
            out << "⊥";
            return out.str();
        }

        out << "{" << std::endl;
        ap_lincons0_array_t ap_csts = ap_abstract0_to_lincons_array(manager(), this->env_.get());

        for (auto i = 0U; i < ap_csts.size;) {
            ap_lincons0_t& ap_cst = ap_csts.p[i];

            if (ap_cst.constyp == AP_CONS_EQMOD) {
                // LinearConstraintT does not support modular equality
                ASSERTC(ap_cst.scalar != nullptr);

                auto expr = toLinearExpr(ap_cst.linexpr0);
                Number mod = impl_::toBNumber<Number>(ap_cst.scalar, caster_);
                out << expr << " = 0 mod " << mod;
            } else {
                auto cst = toLinearConstraint(ap_cst);
                out << cst;
            }

            i++;
            if (i < ap_csts.size) {
                out << "; ";
            }
        }

        ap_lincons0_array_clear(&ap_csts);
        out << "}";
        return out.str();
    }

    std::string toString() const override {
        this->normalize();

        std::stringstream out;

        if (this->isBottom()) {
            out << "⊥";
            return out.str();
        }

        out << "{" << std::endl;
        for (auto&& it : this->vars_) {
            out << util::toString(*it.first) << " = " << toInterval(it.first) << std::endl;
        }

        if (enableOctagonPrint.get(false)) {
            out << dump() << std::endl;
        }

        out << "}";
        return out.str();
    }

    std::string dump() const {
        std::stringstream ss;
        FILE* file = fopen("temp.octagon", "w");
        ASSERTC(file);
        ap_abstract0_fdump(file, manager(), env_.get());
        fclose(file);
        file = fopen("temp.octagon", "rb");

        char line[256];
        while (fgets(line, sizeof(line), file)) {
            /* note that fgets don't strip the terminating \n, checking its
               presence would allow to handle lines longer that sizeof(line) */
            ss << line;
        }
        fclose(file);
        return ss.str();
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        if (isSupported(op)) {
            applyToAExpr(op, x, toAExpr(y), toAExpr(z));
        } else {
            set(x, toInterval(y)->apply(op, toInterval(z)));
        }
    }

    void applyTo(llvm::ArithType op, Variable x, const Number& y, Variable z) {
        if (isSupported(op)) {
            applyToAExpr(op, x, impl_::toAExpr(y), toAExpr(z));
        } else {
            set(x, toInterval(y)->apply(op, toInterval(z)));
        }
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, const Number& z) {
        if (isSupported(op)) {
            applyToAExpr(op, x, toAExpr(y), impl_::toAExpr(z));
        } else {
            set(x, toInterval(y)->apply(op, toInterval(z)));
        }
    }

    void applyTo(llvm::ArithType op, Variable x, const Number& y, const Number& z) {
        if (isSupported(op)) {
            applyToAExpr(op, x, impl_::toAExpr(y), impl_::toAExpr(z));
        } else {
            set(x, toInterval(y)->apply(op, toInterval(z)));
        }
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        // TODO: implement more precise comparison
        return toInterval(x)->apply(op, toInterval(y));
    }

    Ptr applyTo(llvm::ConditionType op, const Number& x, Variable y) {
        // TODO: implement more precise comparison
        return toInterval(x)->apply(op, toInterval(y));
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, const Number& y) {
        // TODO: implement more precise comparison
        return toInterval(x)->apply(op, toInterval(y));
    }

    Ptr applyTo(llvm::ConditionType op, const Number& x, const Number& y) {
        // TODO: implement more precise comparison
        return toInterval(x)->apply(op, toInterval(y));
    }

};

} // namespace apron
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_APRONDOMAIN_HPP
