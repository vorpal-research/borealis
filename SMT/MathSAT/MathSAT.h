/*
 * MathSAT.h
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#ifndef SMT_MATHSAT_H_
#define SMT_MATHSAT_H_

#include <mathsat/mathsat.h>

#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Util/util.h"

namespace borealis {
namespace mathsat {

class Env;
class Sort;
class Decl;
class Config;
class Expr;
class Solver;


typedef std::shared_ptr<msat_config> ConfigPointer;
typedef std::shared_ptr<msat_env> EnvPointer;


static ConfigPointer makeConfigPointer(msat_config* config) {
    auto configDeleter =
        [](msat_config* cfg_) {
            msat_destroy_config(*cfg_);
            delete cfg_;
        };
    return ConfigPointer(config,  configDeleter);
}

static EnvPointer makeEnvPointer(msat_env* env) {
    auto envDeleter =
        [](msat_env* env_) {
            msat_destroy_env(*env_);
            delete env_;
        };
    return EnvPointer(env, envDeleter);
}

////////////////////////////////////////////////////////////////////////////////
// Config == z3::config
////////////////////////////////////////////////////////////////////////////////

class Config {
private:
    ConfigPointer config_;

public:
    Config();
    Config(const std::string& logic);
    Config(FILE* f);

    Config(const Config&) = default;
    Config(Config&&) = default;

    Config& operator=(const Config&) = default;
    Config& operator=(Config&&) = default;

    operator msat_config() const { return *config_; }

    void set(const std::string& option, const std::string& value) {
        msat_set_option(*config_, option.c_str(), value.c_str());
    }
    void set(const std::string& option, bool value) {
        msat_set_option(*config_, option.c_str(), value ? "true" : "false");
    }
    void set(const std::string& option, int value) {
        auto str = util::toString(value);
        msat_set_option(*config_, option.c_str(), str.c_str());
    }

    Env env();
};

////////////////////////////////////////////////////////////////////////////////
// Env == z3::context
////////////////////////////////////////////////////////////////////////////////

class Env {
private:
    Config cfg_;
    EnvPointer env_;

    Env(const Config& cfg, EnvPointer env) : cfg_(cfg), env_(env) {}

public:
    Env(const Config& config);

    Env(const Env& that) = default;
    Env(Env&&) = default;

    Env& operator=(const Env &that) = default;
    Env& operator=(Env&& that) = default;

    operator msat_env() const { return *env_; }

    const Config& config() const { return cfg_; }

    void reset();

    Sort bool_sort();
    Sort int_sort();
    Sort rat_sort();
    Sort bv_sort(unsigned size);
    Sort array_sort(const Sort& idx, const Sort& elm);
    Sort simple_sort(const std::string& name);

    Expr constant(const std::string& name, const Sort& type);
    Expr fresh_constant(const std::string& name, const Sort& type);
    Expr bool_const(const std::string& name);
    Expr int_const(const std::string& name);
    Expr rat_const(const std::string& name);
    Expr bv_const(const std::string& name, const unsigned size);

    Expr bool_val(bool b) const;
    Expr num_val(int i) const;
    Expr bv_val(int i, unsigned size) const;

    Decl function(const std::string& name, const std::vector<Sort>& params, const Sort& ret);
    Decl fresh_function(const std::string& name, const std::vector<Sort>& params, const Sort& ret);

    static Env share(const Env& that);
};

////////////////////////////////////////////////////////////////////////////////
// Sort == z3::sort
////////////////////////////////////////////////////////////////////////////////

class Sort {
private:
    msat_type type_;
    Env env_;

public:
    Sort(const Env& env, const msat_type& type) : type_(type), env_(env) {}

    Sort(const Sort&) = default;
    Sort(Sort&&) = default;

    operator msat_type() const { return type_; }

    bool is_bool() const { return msat_is_bool_type(env_, type_); }
    bool is_int () const { return msat_is_integer_type(env_, type_); }
    bool is_rat() const { return msat_is_rational_type(env_, type_); }
    bool is_arith() const { return is_int() || is_rat(); }
    bool is_bv() const { return msat_is_bv_type(env_, type_, nullptr); }
    bool is_array() const { return msat_is_array_type(env_, type_, nullptr, nullptr); }

    unsigned bv_size() const;

    friend bool operator==(const Sort& a, const Sort& b);
    friend bool operator!=(const Sort& a, const Sort& b);
};

////////////////////////////////////////////////////////////////////////////////
// Decl == z3::func_decl + z3::var_decl (which is none =) )
////////////////////////////////////////////////////////////////////////////////

class Decl {
private:
    Env env_;
    msat_decl decl_;

public:
    Decl(const Env& env, const msat_decl& decl_) : env_(env), decl_(decl_) {}

    operator msat_decl() const { return decl_; }

    const Env& env() const { return env_; }

    unsigned arity() const { return msat_decl_get_arity(decl_); }
    Sort domain(unsigned i) const { return Sort(env_, msat_decl_get_arg_type(decl_, i)); }
    Sort range() const { return Sort(env_, msat_decl_get_return_type(decl_)); }
    std::string name() const { return msat_decl_get_name(decl_); }
    msat_symbol_tag tag() const { return msat_decl_get_tag(env_, decl_); }

    Expr operator()(const std::vector<Expr>& args) const;
    Expr operator()(const Expr& arg1) const;
    Expr operator()(const Expr& arg1, const Expr& arg2) const;
    Expr operator()(const Expr& arg1, const Expr& arg2, const Expr& arg3) const;
};

////////////////////////////////////////////////////////////////////////////////
// Expr == z3::expr
////////////////////////////////////////////////////////////////////////////////

enum class VISIT_STATUS {
    PROCESS,
    SKIP,
    ABORT,
};

using visit_function = std::function<VISIT_STATUS(Expr, void*)>;


class Expr {
private:
    Env env_;
    msat_term term_;

public:
    Expr(const Env &env, const msat_term& term) : env_(env), term_(term) {}
    Expr(const Expr&) = default;
    Expr(Expr&&) = default;

    operator msat_term() const { return term_; }

    const Env& env() const { return env_; }

    Expr& operator=(const Expr&) = default;
    Expr& operator=(Expr&&) = default;

    Sort get_sort() const { return Sort(env_, msat_term_get_type(term_)); }

    bool is_bool() const { return msat_is_bool_type(env_, get_sort()); }
    bool is_int() const { return msat_is_integer_type(env_, get_sort()); }
    bool is_rat() const { return msat_is_rational_type(env_, get_sort()); }
    bool is_bv() const { return msat_is_bv_type(env_, get_sort(), nullptr); }
    bool is_array() const { return msat_is_array_type(env_, get_sort(), nullptr, nullptr); }

    Decl decl() const;
    unsigned num_args() const { return msat_term_arity(term_); }
    Sort arg_sort(unsigned i) const;
    Expr arg(unsigned i) const;

    void visit(visit_function func, void* data);

    friend Expr operator -(const Expr& a);
    friend Expr operator !(const Expr& a);
    friend Expr operator ~(const Expr& a);

#define FRIEND_OP_FOR_BOOL(OP) \
    friend Expr operator OP(const Expr& a, const Expr& b); \
    friend Expr operator OP(const Expr& a, bool b); \
    friend Expr operator OP(bool a, const Expr& b);

    FRIEND_OP_FOR_BOOL(&&);
    FRIEND_OP_FOR_BOOL(||);
    FRIEND_OP_FOR_BOOL(^);

#undef FRIEND_OP_FOR_BOOL

    friend Expr operator ^(const Expr& a, int b);
    friend Expr operator ^(int a, const Expr& b);
    friend Expr operator %(const Expr& a, const Expr& b);

#define FRIEND_OP_FOR_INT(OP) \
    friend Expr OP(const Expr& a, const Expr& b); \
    friend Expr OP(const Expr& a, int b); \
    friend Expr OP(int a, const Expr& b);

    FRIEND_OP_FOR_INT(operator ==);
    FRIEND_OP_FOR_INT(operator !=);
    FRIEND_OP_FOR_INT(operator +);
    FRIEND_OP_FOR_INT(operator -);
    FRIEND_OP_FOR_INT(operator *);
    FRIEND_OP_FOR_INT(operator /);
    FRIEND_OP_FOR_INT(operator >);
    FRIEND_OP_FOR_INT(operator >=);
    FRIEND_OP_FOR_INT(operator <);
    FRIEND_OP_FOR_INT(operator <=);
    FRIEND_OP_FOR_INT(operator &);
    FRIEND_OP_FOR_INT(operator |);

    FRIEND_OP_FOR_INT(ult);
    FRIEND_OP_FOR_INT(ule);
    FRIEND_OP_FOR_INT(ugt);
    FRIEND_OP_FOR_INT(uge);

    FRIEND_OP_FOR_INT(udiv);

#undef FRIEND_OP_FOR_INT

    friend Expr implies(const Expr& a, const Expr& b);
    friend Expr iff(const Expr& a, const Expr& b);
    friend Expr ite(const Expr& cond, const Expr& then_, const Expr& else_);
    friend Expr concat(const Expr& a, const Expr& b);
    friend Expr extract(const Expr& a, unsigned msb, unsigned lsb);
    friend Expr sext(const Expr& a, unsigned amount);
    friend Expr zext(const Expr& a, unsigned amount);
    friend Expr lshl(const Expr& a, const Expr& b);
    friend Expr lshr(const Expr& a, const Expr& b);
    friend Expr ashr(const Expr& a, const Expr& b);
    friend Expr comp(const Expr& a, const Expr& b);
    friend Expr rol(const Expr& a, unsigned b);
    friend Expr ror(const Expr& a, unsigned b);
    friend Expr distinct(const std::vector<Expr>& exprs);

    template<class Streamer>
    friend Streamer& operator<<(Streamer& out, const Expr& e) {
        auto smtlib = util::uniq(msat_to_smtlib2_ext(e.env_, e.term_, nullptr, 0));
        out << smtlib.get();
        // this is generally fucked up
        return static_cast<Streamer&>(out);
    }

    static Expr from_string(Env& env, const std::string& data);
    static Expr from_smtlib(Env& env, const std::string& data);
    static Expr from_smtlib2(Env& env, const std::string& data);

}; // class Expr


#define OP_FOR_INT(OP) \
    Expr OP(const Expr& a, const Expr& b); \
    Expr OP(const Expr& a, int b); \
    Expr OP(int a, const Expr& b);

    OP_FOR_INT(ult);
    OP_FOR_INT(ule);
    OP_FOR_INT(ugt);
    OP_FOR_INT(uge);

#undef OP_FOR_INT

Expr implies(const Expr& a, const Expr& b);
Expr iff(const Expr& a, const Expr& b);
Expr ite(const Expr& cond, const Expr& then_, const Expr& else_);
Expr concat(const Expr& a, const Expr& b);
Expr extract(const Expr& a, unsigned msb, unsigned lsb);
Expr sext(const Expr& a, unsigned amount);
Expr zext(const Expr& a, unsigned amount);
Expr lshl(const Expr& a, const Expr& b);
Expr lshr(const Expr& a, const Expr& b);
Expr ashr(const Expr& a, const Expr& b);
Expr comp(const Expr& a, const Expr& b);
Expr rol(const Expr& a, unsigned b);
Expr ror(const Expr& a, unsigned b);
Expr distinct(const std::vector<Expr>& exprs);

////////////////////////////////////////////////////////////////////////////////
// Solver == z3::solver
////////////////////////////////////////////////////////////////////////////////

class Solver {
private:
    Env env_orig_;
    Env env_;

public:
    typedef int InterpolationGroup;

    explicit Solver(const Env& env) : env_orig_(env), env_(Env::share(env)) {};

    Env& env() { return env_; }

    void add(const Expr&);
    void reset() { env_.reset(); }

    void push();
    void pop();
    unsigned num_backtrack() { return msat_num_backtrack_points(env_); }

    msat_result check() { return msat_solve(env_); }
    msat_result check(const std::vector<Expr>& assumptions);

    std::vector<Expr> assertions();
    std::vector<Expr> unsat_core();

    InterpolationGroup create_interp_group() { return msat_create_itp_group(env_); }
    void set_interp_group(InterpolationGroup gr);
    InterpolationGroup create_and_set_itp_group() {
        auto group = create_interp_group();
        set_interp_group(group);
        return group;
    }
    Expr get_interpolant(const std::vector<InterpolationGroup>& A);
};

} // namespace mathsat
} // namespace borealis

#endif /* SMT_MATHSAT_H_ */
