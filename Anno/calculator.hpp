/*
 * anno.hpp
 *
 *  Created on: Nov 1, 2012
 *      Author: belyaev
 */

#ifndef ANNO_CALCULATOR_HPP
#define ANNO_CALCULATOR_HPP

#include <pegtl.hh>

#include <algorithm>
#include <memory>
#include <sstream>
#include <stack>

#include "Anno/command.hpp"
#include "Anno/locator.hpp"
#include "Anno/production.h"
#include "Anno/string_cast.hpp"
#include "Anno/zappo.hpp"
#include "Util/util.h"

#include "Anno/zappo_macros.h"
#include "Util/macros.h"

namespace borealis {
namespace anno {

// distinguish between std::string and pegtl::string
// std::string -> stdstring
typedef std::string stdstring;

// pegtl::string -> pstring
template<int ...Chars>
struct pstring : pegtl::string< Chars... > {};



// the full set of moving operator-functor versions
// unary ops
template<class T>
struct load :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    { return deref(move(x)); }
};

template<class T>
struct negate :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    { return -move(x); }
};

template<class T>
struct bit_not :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    { return ~move(x); }
};

template<class T>
struct logical_not :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    { return !move(x); }
};

template<class T>
struct indices :
        std::binary_function< T, T, T > {
    T operator()(T&& x, T&& y) const
    { return index(move(x), move(y)); }
};

template<class T>
struct calls :
        std::binary_function< T, T, T > {
    T operator()(T&& x, T&& y) const
    { return call(move(x), move(y)); }
};

template<class T>
struct dots :
        std::binary_function< T, T, T > {
    T operator()(T&& x, T&& y) const
    { return property_access(move(x), move(y)); }
};

template<class T>
struct arrows :
        std::binary_function< T, T, T > {
    T operator()(T&& x, T&& y) const
    { return property_indirect_access(move(x), move(y)); }
};

template<class T>
struct implies :
        std::binary_function< T, T, T > {
    T operator()(T&& x, T&& y) const
    { return imply(move(x), move(y)); }
};

// binary ops
#define DEFBINARY(NAME, OP) \
    template <class T> \
    struct NAME : std::binary_function < T, T, T > { \
        T operator()(T&& x, T&& y) const \
        { return move(x) OP move(y); } \
    };

DEFBINARY( pluss,            + )
DEFBINARY( minus,            - )
DEFBINARY( multiplies,       * )
DEFBINARY( divides,          / )
DEFBINARY( modulus,          % )
DEFBINARY( left_shift,       << )
DEFBINARY( right_shift,      >> )
DEFBINARY( greater,          > )
DEFBINARY( less,             < )
DEFBINARY( greater_or_equal, >= )
DEFBINARY( less_or_equal,    <= )
DEFBINARY( equal,            == )
DEFBINARY( nequal,           != )
DEFBINARY( bit_and,          & )
DEFBINARY( bit_xor,          ^ )
DEFBINARY( bit_or,           | )
DEFBINARY( logical_and,      && )
DEFBINARY( logical_or,       || )

#undef DEFBINARY // good housekeeping

namespace calculator {

typedef command command_type;
typedef prod_t expression_type;
// The state for the arithmetics
typedef std::stack< expression_type > expr_stack;
typedef std::vector< command > commands_t;

using pegtl::action_base;
using pegtl::alpha;
using pegtl::digit;
using pegtl::identifier;
using pegtl::ifapply;
using pegtl::ifmust;
using pegtl::one;
using pegtl::opt;
using pegtl::pad;
using pegtl::plus;
using pegtl::range;
using pegtl::seq;
using pegtl::sor;
using pegtl::space;
using pegtl::space_until_eof;
using pegtl::star;
using pegtl::xdigit;

template<typename ExprType>
ExprType pull(std::stack< ExprType >& s) {
    ASSERTC( !s.empty() );
    ExprType nrv(std::move(s.top()));
    s.pop();
    return std::move(nrv);
}

template<class Prim>
struct push :
        action_base< push< Prim > > {
    static void apply(const std::string& m, expr_stack& s, command_type&, commands_t&) {
        s.push(productionFactory::bind(string_cast< Prim >(m)));
    }
};

struct mask_push :
        action_base< mask_push > {
    static void apply(const std::string& m, expr_stack& s, command_type&, commands_t&) {
        s.push(productionFactory::createMask(m));
    }
};

struct store_command :
        action_base< store_command > {
    static void apply(const std::string&, expr_stack& s, command_type& com, commands_t& coms) {
        while (!s.empty()) s.pop();
        coms.push_back(std::move(com));
    }
};

struct op_collect_list:
        action_base<op_collect_list> {

    static void apply(const std::string&, expr_stack& s, command_type&, commands_t&) {
        auto rhs = pull(s);
        auto lhs = pull(s);
        s.push(productionFactory::createList(std::move(lhs), std::move(rhs)));
    }
};

// Class op_baction performs an operation on the two top-most elements of
// the evaluation stack. This should always be possible in the sense that
// the grammar must make sure to only apply this action when sufficiently
// many operands are on the stack.
template<typename Operation>
struct op_baction :
        action_base< op_baction< Operation > > {
    static void apply(const std::string&, expr_stack& s, command_type&, commands_t&) {
        auto rhs = pull(s);
        auto lhs = pull(s);
        s.push(Operation()(std::move(lhs), std::move(rhs)));
    }
};

// Class op_uaction performs an operation on the top-most element of
// the evaluation stack. This should always be possible in the sense that
// the grammar must make sure to only apply this action when sufficiently
// many operands are on the stack.
template<typename Operation>
struct op_uaction :
        action_base< op_uaction< Operation > > {
    static void apply(const std::string&, expr_stack& s, command_type&, commands_t&) {
        auto arg = pull(s);
        s.push(Operation()(std::move(arg)));
    }
};

struct ask_keyword :
        action_base< ask_keyword > {
    static void apply(const std::string& value, const expr_stack&, command_type& comm, commands_t&) {
        comm.name_ = borealis::util::nospaces(stdstring(value));
    }
};

struct ask_meta :
        action_base< ask_meta > {
    static void apply(const std::string& value, const expr_stack&, command_type& comm, commands_t&) {
        comm.meta_ = borealis::util::nospaces(stdstring(value));
    }
};

struct ask_arguments :
        action_base< ask_arguments > {
    static void apply(const std::string&, expr_stack& s, command_type& comm, commands_t&) {
        if (comm.name_ == "ignore" || comm.name_ == "inline" || comm.name_ == "skip" || comm.name_ == "endmask") {
            ASSERTC(s.empty());
        } else if (comm.name_ == "mask") {
            while (!s.empty()) {
                comm.args_.push_front(std::move(s.top()));
                s.pop();
            }
        } else {
            ASSERTC(s.size() == 1);
            comm.args_.push_front(std::move(s.top()));
            s.pop();
        }
    }
};

}   // namespace calculator

}   // namespace anno
}   // namespace borealis

#include "Util/unmacros.h"
#include "Anno/zappo_unmacros.h"

#endif // ANNO_CALCULATOR_HPP
