//
// Created by belyaev on 6/6/16.
//

#ifndef AURORA_SANDBOX_ENGINE_H
#define AURORA_SANDBOX_ENGINE_H

namespace borealis {

template<class T, class Ix, Ix size>
struct stdarray_wrapper {
    static constexpr size_t index_cast(Ix ix) {
        return static_cast<size_t>(static_cast<std::underlying_type_t<Ix>>(ix));
    }

    std::array<T, index_cast(size)> data;

    T& operator[](Ix ix) {
        return data[ index_cast(ix) ];
    }
    const T& operator[](Ix ix) const {
        return data[ index_cast(ix) ];
    }

    T* begin() { return data.begin(); }
    T* end() { return data.end(); }
    const T* begin() const { return data.begin(); }
    const T* end() const { return data.end(); }
};

struct SmtEngine {

    struct stub{};
    enum class freshness: bool{ fresh, normal };

    using expr_t = stub;
    using sort_t = stub;
    using function_t = stub;

    using factory_t = stub;
    using context_t = stub;
    using solver_t = stub;

    static size_t hash(expr_t) { return 0; }
    static std::string name(expr_t) { return ""; }
    static std::string toString(expr_t) { return ""; }
    static bool term_equality(context_t&, expr_t, expr_t) { return true; }
    static sort_t get_sort(context_t&, expr_t){ return {}; }

    static sort_t bool_sort(context_t&){ return {}; }
    static sort_t bv_sort(context_t&, size_t){ return {}; }
    static sort_t array_sort(context_t&, sort_t, sort_t){ return {}; }

    static expr_t mkVar(context_t&, sort_t, const std::string&, freshness);
    static expr_t mkBoolConst(context_t&, bool) { return {}; }
    static expr_t mkNumericConst(context_t&, sort_t, int64_t){ return {}; }
    static expr_t mkNumericConst(context_t&, sort_t, uint64_t){ return {}; }
    static expr_t mkNumericConst(context_t&, sort_t, const std::string&){ return {}; }

    static expr_t mkArrayConst(context_t&, sort_t, expr_t){ return {}; }
    static function_t mkFunction(context_t&, const std::string&, sort_t, const std::vector<sort_t>& ) { return {}; }

    enum class unOp {
        NEGATE = 0, BNOT, UMINUS, LAST
    };

    enum class binOp {
        CONJ = 0,
        DISJ,
        IMPLIES,
        IFF,
        CONCAT,
        BAND,
        BOR,
        XOR,
        ADD,
        SUB,
        SMUL,
        SDIV,
        SMOD,
        SREM,
        UMUL,
        UDIV,
        UMOD,
        UREM,
        ASHL,
        ASHR,
        LSHR,
        LOAD,
        EQUAL,
        NEQUAL,
        SGT,
        SLT,
        SGE,
        SLE,
        UGT,
        ULT,
        UGE,
        ULE,
        LAST
    };

    static expr_t binop(context_t&, binOp, expr_t, expr_t) { return {}; }
    static expr_t unop(context_t&, unOp, expr_t) { return {}; }

    static expr_t ite(context_t&, expr_t, expr_t, expr_t) { return {}; }

    static expr_t extract(context_t&, expr_t, size_t, size_t){ return {}; }
    static expr_t sext(context_t&, size_t, expr_t) { return {}; }
    static expr_t zext(context_t&, size_t, expr_t) { return {}; }
    static expr_t store(context_t&, expr_t, expr_t, expr_t){ return {}; }
    static expr_t apply(context_t&, function_t, const std::vector<expr_t>&){ return {}; }
};


} /* namespace borealis */

#endif //AURORA_SANDBOX_ENGINE_H
