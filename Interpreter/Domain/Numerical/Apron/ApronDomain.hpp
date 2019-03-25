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

using ApronPtr = std::shared_ptr<ap_abstract0_t>;

enum DomainT {
    INTERVAL,
    OCTAGON
};

inline ap_manager_t* allocate_manager(DomainT domain) {
    switch (domain) {
        case INTERVAL: return box_manager_alloc();
        case OCTAGON: return oct_manager_alloc();
        default:
            UNREACHABLE("Unknown apron domain");
    }
}

inline ApronPtr newPtr(ap_abstract0_t* domain) {
    return ApronPtr{ domain, LAM(a, ap_abstract0_free(ap_abstract0_manager(a), a)) };
}

inline size_t dims(ap_abstract0_t* ptr) {
    return ap_abstract0_dimension(ap_abstract0_manager(ptr), ptr).intdim;
}

inline ApronPtr addDimensions(ap_abstract0_t* ptr, size_t dims) {
    ASSERTC(dims > 0);

    auto* dim_change = ap_dimchange_alloc(dims, 0);
    for (auto i = 0U; i < dims; ++i) {
        dim_change->dim[i] = static_cast<ap_dim_t>(apron::dims(ptr));
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

template <bool sign>
inline ap_texpr0_t* toAExpr(const BitInt<sign>& n) {
    mpq_class e(n.toGMP());
    return ap_texpr0_cst_scalar_mpq(e.get_mpq_t());
}

template <>
inline ap_texpr0_t* toAExpr(const Float& n) {
    mpq_class e(n.toGMP());
    return ap_texpr0_cst_scalar_mpq(e.get_mpq_t());
}

template < typename Number >
inline Number toBNumber(ap_scalar_t*, util::Adapter<Number>* caster);

template <bool sign>
inline BitInt<sign> toBNumber(ap_scalar_t* scalar, util::Adapter<BitInt<sign>>* caster) {
    ASSERTC(ap_scalar_infty(scalar) == 0);
    ASSERTC(scalar->discr == AP_SCALAR_MPQ);

    auto q = mpq_class(scalar->val.mpq);

    std::stringstream ss;
    ss << q;

    return (*caster)(ss.str());
}

template <>
inline Float toBNumber(ap_scalar_t* scalar, util::Adapter<Float>* caster) {
    ASSERTC(ap_scalar_infty(scalar) == 0);
    ASSERTC(scalar->discr == AP_SCALAR_MPQ);

    auto q = mpq_class(scalar->val.mpq);
    std::stringstream ss;
    ss << q;

    return (*caster)(ss.str());
}

template < typename Number >
inline Number toBNumber(ap_coeff_t* coeff, util::Adapter<Number>* caster) {
    ASSERTC(coeff->discr == AP_COEFF_SCALAR);

    return toBNumber<Number>(coeff->val.scalar, caster);
}

template < typename Number >
inline Bound<Number> toBBound(ap_scalar_t* scalar, util::Adapter<Number>* caster) {
    using BoundT = Bound<Number>;

    if (ap_scalar_infty(scalar) == -1) {
        return BoundT::minusInfinity(caster);
    } else if (ap_scalar_infty(scalar) == 1) {
        return BoundT::plusInfinity(caster);
    } else {
        return BoundT(caster, toBNumber<Number>(scalar, caster));
    }
}

template < typename Number >
inline Interval<Number> toBInterval(ap_interval_t* intv, util::Adapter<Number>* caster) {
    using IntervalT = Interval<Number>;

    if (ap_interval_is_top(intv)) {
        return IntervalT::top(caster);
    }

    if (ap_interval_is_bottom(intv)) {
        return IntervalT::bottom(caster);
    }

    return IntervalT::interval(toBBound<Number>(intv->inf, caster), toBBound<Number>(intv->sup, caster));
}

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

template <typename Number, typename Variable, typename VarHash, typename VarEquals, apron::DomainT domain>
class ApronDomain : public NumericalDomain<Variable> {
public:
    using Self = ApronDomain<Number, Variable, VarHash, VarEquals, domain>;
    using Ptr = AbstractDomain::Ptr;
    using ConstPtr = AbstractDomain::ConstPtr;

    using IntervalT = Interval<Number>;
    using LinearConstSystemT = LinearConstraintSystem<Number, Variable, VarHash, VarEquals>;
    using LinearConstraintT = typename LinearConstSystemT::LinearConstraintT;
    using LinearExprT = typename LinearConstSystemT::LinearExpr;

private:

    using VariableMap = std::unordered_map<Variable, size_t, VarHash, VarEquals>;

private:

    VariableMap vars_;
    apron::ApronPtr env_;

private:

    static ap_manager_t* manager() {
        static auto* m = apron::allocate_manager(domain);
        return m;
    }

    util::option<size_t> getOrPutVar(Variable x) const {
        return util::at(vars_, x);
    }

    size_t addVar(Variable x) {
        auto&& opt = getOrPutVar(x);
        if (opt) return opt.getUnsafe();

        auto num = static_cast<ap_dim_t>(vars_.size());
        env_ = addDimensions(env_.get(), 1);
        vars_.insert_or_assign(x, num);

        ASSERTC(vars_.size() == dims(env_.get()));
        return num;
    }

    static ap_dimperm_t* build_perm_map(const VariableMap& old_map,
                                        const VariableMap& new_map) {
        std::size_t n = new_map.size();
        ap_dimperm_t* perm = ap_dimperm_alloc(n);
        std::vector<bool> index_assigned(n, false);
        std::vector<bool> value_assigned(n, false);

        for (auto it = old_map.begin(); it != old_map.end(); ++it) {
            auto&& opt = util::at(new_map, it.first);
            ASSERTC(opt);
            auto dim = opt.getUnsafe();

            perm->dim[it->second] = dim;
            index_assigned[it->second] = true;
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
                                      apron::ApronPtr& inv_x,
                                      const VariableMap& var_map_y,
                                      apron::ApronPtr& inv_y) {
        ikos_assert(var_map_x.size() == apron::dims(inv_x.get()));
        ikos_assert(var_map_y.size() == apron::dims(inv_y.get()));

        VariableMap result_var_map(var_map_x);
        bool equal = (var_map_x.size() == var_map_y.size());

        for (auto it = var_map_y.begin(); it != var_map_y.end(); ++it) {
            auto&& v_y = it.first;
            auto&& dim_y = it.second;
            auto&& dim_x = util::at(var_map_x, v_y);
            equal = equal && dim_x && dim_x.getUnsafe() == dim_y;

            if (not dim_x) {
                auto dim = static_cast<ap_dim_t>(result_var_map.size());
                result_var_map.insert_or_assign(v_y, dim);
            }
        }

        if (equal) {
            return result_var_map;
        }

        if (result_var_map.size() > var_map_x.size()) {
            inv_x = apron::addDimensions(inv_x.get(), result_var_map.size() - var_map_x.size());
        }
        if (result_var_map.size() > var_map_y.size()) {
            inv_y = apron::addDimensions(inv_y.get(), result_var_map.size() - var_map_y.size());
        }

        ASSERTC(result_var_map.size() == apron::dims(inv_x.get()));
        ASSERTC(result_var_map.size() == apron::dims(inv_y.get()));

        auto* perm_y = build_perm_map(var_map_y, result_var_map);
        inv_y = apron::newPtr(ap_abstract0_permute_dimensions(manager(), false, inv_y.get(), perm_y));
        ap_dimperm_free(perm_y);

        ASSERTC(result_var_map.size() == apron::dims(inv_x.get()));
        ASSERTC(result_var_map.size() == apron::dims(inv_y.get()));

        return result_var_map;
    }

    ap_texpr0_t* toAExpr(Variable x) {
        return ap_texpr0_dim(getOrPutVar(x));
    }

    ap_texpr0_t* toAExpr(const LinearExprT& le) {
        auto* r = toAExpr(le.constant());

        for (auto&& it : le) {
            auto* term = binopExpr<Number>(AP_TEXPR_MUL, toAExpr(it->second), toAExpr(it->first));
            r = binopExpr<Number>(AP_TEXPR_ADD, r, term);
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

public:
    explicit ApronDomain(TopTag) : NumericalDomain<Variable>(class_tag(*this)), env_(newPtr(ap_abstract0_top(manager(), 0, 0))) {}
    explicit ApronDomain(BottomTag) : NumericalDomain<Variable>(class_tag(*this)), env_(newPtr(ap_abstract0_bottom(manager(), 0, 0))) {}

    ApronDomain() : ApronDomain(TopTag{}) {}
    ApronDomain(VariableMap vars, ApronPtr env) : NumericalDomain<Variable>(class_tag(*this)), vars_(std::move(vars)), env_(env) {}
    ApronDomain(const ApronDomain&) = default;
    ApronDomain(ApronDomain&&) = default;
    ApronDomain& operator=(const ApronDomain&) = default;
    ApronDomain& operator=(ApronDomain&&) = default;
    virtual ~ApronDomain() = default;

    static Ptr top() { return std::make_shared<ApronDomain>(TopTag{}); }
    static Ptr bottom() { return std::make_shared<ApronDomain>(BottomTag{}); }

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
        this->operator=(*unwrap(top()));
    }

    void setBottom() override {
        this->operator=(*unwrap(bottom()));
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
            ApronPtr inv_x(this->env_);
            ApronPtr inv_y(otherRaw->env_);
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
            ApronPtr inv_x(this->env_);
            ApronPtr inv_y(otherRaw->env_);
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
            ApronPtr inv_x(this->env_);
            ApronPtr inv_y(otherRaw->env_);
            VariableMap var_map = merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            this->env_ = newPtr(ap_abstract0_join(manager(), false, inv_x.get(), inv_y.get()));
        }
    }

    void meetWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || other->isTop()) {
            return;
        } else if (this->isTop() || other->isBottom()) {
            this->operator=(*otherRaw);
        } else {
            ApronPtr inv_x(this->env_);
            ApronPtr inv_y(otherRaw->env_);
            VariableMap var_map = merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            this->env_ = newPtr(ap_abstract0_meet(manager(), false, inv_x.get(), inv_y.get()));
        }
    }

    void widenWith(ConstPtr other) {
        auto* otherRaw = unwrap(other);

        if (this->isBottom() || other->isTop()) {
            this->operator=(*otherRaw);
        } else if (this->isTop() || other->isBottom()) {
            return;
        } else {
            ApronPtr inv_x(this->env_);
            ApronPtr inv_y(otherRaw->env_);
            VariableMap var_map = merge_var_maps(this->vars_, inv_x, otherRaw->vars_, inv_y);
            this->env_ = newPtr(ap_abstract0_widening(manager(), inv_x.get(), inv_y.get()));
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
        auto&& has_dim = getOrPutVar(x);

        if (not has_dim) {
            return;
        }

        auto dim = has_dim.getUnsafe();
        std::vector< ap_dim_t > vector_dims{dim};
        this->env_ = newPtr(ap_abstract0_forget_array(manager(), false,
                this->env_.get(), &vector_dims[0], vector_dims.size(), false));

        this->env_ = removeDimensions(this->env_.get(), vector_dims);
        this->vars_ = util::viewContainer(vars_).map(
                        [&](const std::pair<Variable, size_t>& it) -> util::option<std::pair<Variable, size_t>> {
                            if (it.second < dim) it;
                            else if (it.second == dim) util::nothing();
                            else std::make_pair(it.first, it.second - 1);
                        })
                        .filter(LAM(a, a))
                        .map(LAM(a, a.getUnsafe()))
                        .template toHashMap<std::pair<Variable, size_t>, Variable, size_t, VarHash, VarEquals>();
        ASSERTC(this->vars_.size() == dims(this->env_.get()));
    }

    void refine(Variable x, const IntervalT& interval) {
        if (this->isBottom()) {
            return;
        } else if (interval.isBottom()) {
            this->setBottom();
        } else if (interval.isTop()) {
            return;
        } else {
            this->add(LinearConstSystemT::withinInterval(x, interval));
        }
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
            ap_csts.p[i++] = to_ap_constraint(cst);
        }

        this->_inv = newPtr(ap_abstract0_meet_tcons_array(manager(), false, this->env_.get(), &ap_csts));

        // this step allows to improve the precision
        for (i = 0; i < csts.size() && !this->is_bottom(); i++) {
            // check satisfiability of ap_csts.p[i]
            ap_tcons0_t& cst = ap_csts.p[i];
            auto* ap_intv = ap_abstract0_bound_texpr(manager(), this->env_.get(), cst.texpr0);
//            IntervalT intv = apron::to_ikos_interval< Number >(ap_intv);
//
//            if (intv.is_bottom() ||
//                (cst.constyp == AP_CONS_EQ && !intv.contains(0)) ||
//                (cst.constyp == AP_CONS_SUPEQ && intv.ub() < BoundT(0)) ||
//                (cst.constyp == AP_CONS_DISEQ && intv == IntervalT(0))) {
//                // cst is not satisfiable
//                this->set_to_bottom();
//                return;
//            }
//
//            ap_interval_free(ap_intv);
        }

        ap_tcons0_array_clear(&ap_csts);
    }

    Ptr toInterval(Variable x) const override {
        UNREACHABLE("TODO");
    }

    void assign(Variable x, Variable y) override {
        UNREACHABLE("TODO");
    }

    void assign(Variable x, Ptr i) override {
        UNREACHABLE("TODO");
    }

    void assign(Variable x, const LinearExprT& expr) override {
        if (this->isBottom()) {
            return;
        }

        auto* t = toAExpr(expr);
        auto vDim = getOrPutVar(x);
        this->_inv = newPtr(ap_abstract0_assign_texpr(manager(), false, this->env_.get(), vDim, t, nullptr));
        ap_texpr0_free(t);
    }

    void applyTo(llvm::ArithType op, Variable x, Variable y, Variable z) override {
        UNREACHABLE("TODO");
    }

    Ptr applyTo(llvm::ConditionType op, Variable x, Variable y) override {
        UNREACHABLE("TODO");
    }

    size_t hashCode() const override {
        UNREACHABLE("TODO");
    }

    std::string toString() const override {
        UNREACHABLE("TODO");
    }

};

} // namespace apron
} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_APRONDOMAIN_HPP
