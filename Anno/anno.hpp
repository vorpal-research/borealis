#pragma once

#ifndef ANNO_HPP
#define ANNO_HPP

#include <pegtl.hh>

#include "production.h"
#include "command.hpp"
#include "locator.hpp"
#include "../util.h"

#include <stack>
#include <memory>
#include <sstream>
#include <algorithm>

namespace borealis {
namespace anno {

using std::unique_ptr;
using std::istringstream;
using std::vector;
using std::for_each;

using borealis::util::nospaces;

// designate between std::string and pegtl::string
// std::string -> stdstring
typedef std::string stdstring;
// pegtl::string -> pstring
template<int ...Chars>
struct pstring :
        pegtl::string< Chars... > {
};

// bad string cast exception, no additional info provided, sorry
class bad_string_cast :
        public std::exception {
};
// an utility class used to cast string to anything
// it is generally NOT exception-safe iff T is not a POD
// T also needs to have a default constructor
template<class T>
struct string_caster {
    // default is to read it using sstreams
    inline static T apply(const string& sexpr) {
        try {
            istringstream is(sexpr);
            T val;
            is >> val;
            if (!is) throw 0; // will be caught by catch
            return val;
        } catch (...) {
            throw bad_string_cast();
        }
        return T(); // is never reached
    }
};
// string instance is just an id and, consequently, exception-safe
// can create dangling references though
template<>
struct string_caster< string > {
    inline static const string& apply(const string& sexpr) {
        return sexpr;
    }
};
// bool instance needs boolalpha OR can be done manually
// the value MUST be either 'true' or 'false' or it will destroy the program
// (unless u use the catch(...), of course)
template<>
struct string_caster< bool > {
    inline static bool apply(const string& sexpr) {
        if (sexpr == "true")
            return true;
        else if (sexpr == "false")
            return false;
        else
            throw bad_string_cast();
    }
};
// we use long long as a spec 'cos it is used in the parser
// for a general use, it must be an int
// this version checks the full C numeric literal grammar with hex or oct qualifiers
template<>
struct string_caster< long long > {
    inline static long long apply(const string& sexpr) {
        try {
            auto mod = std::dec;
            auto shift = 0;
            if (sexpr.substr(0, 2) == "0x") {
                mod = std::hex;
                shift = 2;
            }
            // seems to be replaced by sexpr[0] == '0', but substr is safer
            else if (sexpr.substr(0, 1) == "0" && sexpr != "0") {
                mod = std::oct;
                shift = 1;
            }
            // we rely on shift being 0 if the string is empty
            istringstream is(sexpr.c_str() + shift);
            long long val;
            is >> mod >> val;
            if (!is) throw 0;  // will be caught by catch
            return val;
        } catch (...) {
            throw bad_string_cast();
        }
        return -1; // is never reached
    }
};

// the casting function itself
// throws bad_string_cast
template<class T>
T string_cast(const string& sexpr) {
    return string_caster< T >::apply(sexpr);
}

// the full set of moving operator-functor versions
// unary ops
template<class T>
struct negate :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    {   return -move(x);}
};

template<class T>
struct bit_not :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    {   return ~move(x);}
};

template<class T>
struct logical_not :
        std::unary_function< T, T > {
    T operator()(T&& x) const
    {   return !move(x);}
};
// binary
#define DEFBINARY(NAME, OP) \
	template <class T> \
	struct NAME : std::binary_function <T,T,T> { \
	  T operator() (T&& x, T&& y) const \
		{return move(x) OP move(y);} \
	};

DEFBINARY( pluss, +)
DEFBINARY( minus, -)
DEFBINARY( multiplies, *)
DEFBINARY( divides, /)
DEFBINARY( modulus, %)
DEFBINARY( left_shift, <<)
DEFBINARY( right_shift, >>)
DEFBINARY( greater, >)
DEFBINARY( less, <)
DEFBINARY( greater_or_equal, >=)
DEFBINARY( less_or_equal, <=)
DEFBINARY( equal, ==)
DEFBINARY( nequal, !=)
DEFBINARY( bit_and, &)
DEFBINARY( bit_xor, ^)
DEFBINARY( bit_or, |)
DEFBINARY( logical_and, &&)
DEFBINARY( logical_or, ||)

#undef DEFBINARY // good housekeeping
namespace calculator {
// The first program used during development and debugging
// of the library that uses actions. It evaluates each command
// line argument as arithmetic expression consisting of
// - integers with optional sign,
// - the four basic arithmetic operations,
// - grouping brackets.
// For example input "3 * ( -7 + 9)" yields result 6.

using namespace pegtl;

// The state for the arithmetics.
typedef prod_t expression_type;
typedef std::stack< expression_type > expr_stack;
typedef command command_type;
typedef vector<command> commands_t;

template<typename ExprType>
ExprType pull(std::stack< ExprType >& s) {
    assert( ! s.empty());
    ExprType nrv(move(s.top()));
    s.pop();
    return nrv;
}

// The actions.

// This action converts the matched sub-string to an integer and pushes it on
// the stack, which must be its only additional state argument.

// Deriving from action_base<> is necessary since version 0.26; the base class
// takes care of the pretty-printing for diagnostic messages; this is necessary
// for all action classes (that do not derive from a rule class).

template<class Prim>
struct push :
        action_base< push< Prim > > {
    static void apply(const std::string & m, expr_stack & s, command_type &, commands_t& ) {
        s.push(productionFactory::bind(string_cast< Prim >(m)));
    }
};

struct mask_push :
        action_base< mask_push > {
    static void apply(const std::string & m, expr_stack & s, command_type &, commands_t& ) {
        s.push(productionFactory::createMask(m));
    }
};

struct store_command :
		action_base< store_command > {
	static void apply(const std::string &, expr_stack & s, command_type & com, commands_t& coms) {
		while(!s.empty()) s.pop();
		coms.push_back(std::move(com));
	}
};

// Class op_baction performs an operation on the two top-most elements of
// the evaluation stack. This should always be possible in the sense that
// the grammar must make sure to only apply this action when sufficiently
// many operands are on the stack.

template<typename Operation>
struct op_baction :
        action_base< op_baction< Operation > > {
    static void apply(const std::string &, expr_stack & s, command_type &, commands_t& ) {
        auto rhs = pull(s);
        auto lhs = pull(s);
        s.push(Operation()(move(lhs), move(rhs)));
    }
};

// Class op_uaction performs an operation on the top-most element of
// the evaluation stack. This should always be possible in the sense that
// the grammar must make sure to only apply this action when sufficiently
// many operands are on the stack.
template<typename Operation>
struct op_uaction :
        action_base< op_uaction< Operation > > {
    static void apply(const std::string &, expr_stack & s, command_type &, commands_t& ) {
        auto arg = pull(s);
        s.push(Operation()(move(arg)));
    }
};

struct ask_keyword :
        action_base< ask_keyword > {
    static void apply(const std::string & value, const expr_stack &, command_type & comm, commands_t& ) {
        comm.name_ = nospaces(stdstring(value));
    }
};

struct ask_arguments :
        action_base< ask_arguments > {
    static void apply(const std::string &, expr_stack & s, command_type & comm, commands_t& ) {
        using std::cerr;
        using std::endl;
        if (comm.name_ == "requires" || comm.name_ == "ensures" || comm.name_ == "assigns"
                || comm.name_ == "assert" || comm.name_ == "stack-depth") {
            assert(s.size() == 1);
            comm.args_.push_front(move(s.top()));
            s.pop();
        } else if (comm.name_ == "ignore" || comm.name_ == "skip" || comm.name_ == "endmask") {
            assert(s.size() == 0);
        } else if (comm.name_ == "mask") {
            while (!s.empty()) {
                comm.args_.push_front(move(s.top()));
                s.pop();
            }
        }
    }
};

// integer grammar
struct integer :
        sor< ifmust< seq< one< '0' >, one< 'x', 'X' > >, plus< xdigit > >, // hex
                ifmust< one< '0' >, star< range< '0', '7' > > >,        // oct
                seq< range< '1', '9' >, star< digit > >               // dec
        > {
};
// --//-- with action applied
struct push_integer :
        pad< ifapply< integer, push< long long > >, space > {
};

// boolean grammar
// must come BEFORE the variable grammar, as 'true' and 'false' are valid var names
// boolean := true | false
struct boolean :
        seq< sor< pstring< 't', 'r', 'u', 'e' >, pstring< 'f', 'a', 'l', 's', 'e' > > > {
};
// --//-- with action applied
struct push_boolean :
        pad< ifapply< boolean, push< bool > >, space > {
};

// FP grammar
// exponent := ('e'|'E') ('+'|'-') +(DIGIT)
struct exponent :
        ifmust< one< 'e', 'E' >, one< '+', '-' >, plus< digit > > {
};
// floating := +(DIGIT) exponent                    // 1e+2
// floating := *(DIGIT) '.' +(DIGIT) ?exponent      // .23
// floating := +(DIGIT) '.' *(DIGIT) ?exponent      // 1.
struct floating :
        sor<
            seq< plus< digit >, exponent >,
            seq< star< digit >, one< '.' >,
            plus< digit >, opt< exponent > >,
            seq<
                plus< digit >,
                one< '.' >,
                star< digit >,
                opt< exponent >
            >
        > {};
// --//-- with action applied
struct push_floating :
        pad< ifapply< floating, push< double > >, space > {
};

// variable is just an identifier, can push it right away
struct push_variable :
        pad< ifapply< identifier, push< stdstring > >, space > {
};
// builtin is just an identifier prefixed with '\'
struct push_builtin :
        pad< ifapply< seq< one< '\\' >, identifier >, push< stdstring > >, space > {
};

// a primitive is a variable or a literal
struct push_primitive :
        sor< push_floating, push_integer, push_boolean, push_variable, push_builtin > {
};

// smth surrounded by spaces
template<typename Rule>
struct literal :
        pad< Rule, space > {
};

// a char surrounded by spaces
template<int C>
struct chpad :
        literal< one< C > > {
};
// a string surrounded by spaces
template<int ...Chars>
struct strpad :
        literal< pstring< Chars... > > {
};

// opening paren
struct read_open :
        chpad< '(' > {
};
// closing paren
struct read_close :
        chpad< ')' > {
};

// forward decl for the whole expression
struct read_expr;
// an atom is a primitive or a parentified expr
struct read_atom :
        sor< push_primitive, seq< read_open, read_expr, read_close > > {
};

// binary operation rule with oper sign being one char
// eq to:
//    bop := 'P' O with action A
template<int P, typename O, typename A>
struct read_bop :
        ifapply< seq< chpad< P >, O >, op_baction< A > > {
};
// generic binary operation rule
// eq to:
//    bop := Rule O with action A
template<typename Rule, typename O, typename A>
struct read_bop_m {
};
template<typename O, typename A, int ...Chars>
struct read_bop_m< pstring< Chars... >, O, A > :
        ifapply< seq< strpad< Chars... >, O >, op_baction< A > > {
};

// unary operation rule
// eq to:
//    uop := 'P' O with action A
template<int P, typename O, typename A>
struct read_uop :
        ifapply< seq< chpad< P >, O >, op_uaction< A > > {
};

// neg := '-' atom
struct read_neg :
        read_uop< '-', read_atom, negate< expression_type > > {
};
// not := '!' atom
struct read_not :
        read_uop< '!', read_atom, logical_not< expression_type > > {
};
// bnot := '~' atom
struct read_bnot :
        read_uop< '~', read_atom, bit_not< expression_type > > {
};

// an unary op is just an atom, neg, not or bnot
struct read_unary :
        sor< read_neg, read_not, read_bnot, read_atom > {
};

// mul := '*' unary
struct read_mul :
        read_bop< '*', read_unary, multiplies< expression_type > > {
};
// div := '/' unary
struct read_div :
        read_bop< '/', read_unary, divides< expression_type > > {
};
// div := '%' unary
struct read_mod :
        read_bop< '%', read_unary, modulus< expression_type > > {
};
// prod = unary *(mul | div | mod)
struct read_prod :
        seq< read_unary, star< sor< read_mul, read_div, read_mod > > > {
};

// add := '+' prod
struct read_add :
        read_bop< '+', read_prod, pluss< expression_type > > {
};
// add := '-' prod
struct read_sub :
        read_bop< '-', read_prod, minus< expression_type > > {
};
// sum := prod *(add | sub)
struct read_sum :
        seq< read_prod, star< sor< read_add, read_sub > > > {
};

// lsh := '<<' sum
struct read_lsh :
        read_bop_m< pstring< '<', '<' >, read_sum, left_shift< expression_type > > {
};
// rsh := '>>' sum
struct read_rsh :
        read_bop_m< pstring< '>', '>' >, read_sum, right_shift< expression_type > > {
};
// shift := sum *(lsh | rsh)
struct read_shift :
        seq< read_sum, star< sor< read_lsh, read_rsh > > > {
};

// gt := '>' shift
struct read_gt :
        read_bop< '>', read_shift, greater< expression_type > > {
};
// lt := '<' shift
struct read_lt :
        read_bop< '<', read_shift, less< expression_type > > {
};
// lt := '>=' shift
struct read_ge :
        read_bop_m< pstring< '>', '=' >, read_shift, greater_or_equal< expression_type > > {
};
// lt := '<=' shift
struct read_le :
        read_bop_m< pstring< '<', '=' >, read_shift, less_or_equal< expression_type > > {
};
// inequality := shift *(gt | lt | ge | le)
struct read_inequality :
        seq< read_shift, star< sor< read_ge, read_gt, read_le, read_lt > > > {
};

// eq = '==' inequality
struct read_eq :
        read_bop_m< pstring< '=', '=' >, read_inequality, equal< expression_type > > {
};
// neq = '==' inequality
struct read_neq :
        read_bop_m< pstring< '!', '=' >, read_inequality, nequal< expression_type > > {
};
// equality := inequality *(eq | neq)
struct read_equality :
        seq< read_inequality, star< sor< read_eq, read_neq > > > {
};

// bitand := '&' equality
struct read_bitand :
        read_bop< '&', read_equality, bit_and< expression_type > > {
};
// bitand2 := equality *(bitand)
struct read_bitand2 :
        seq< read_equality, star< read_bitand > > {
};

// bitxor := '^' bitand2
struct read_bitxor :
        read_bop< '^', read_bitand2, bit_xor< expression_type > > {
};
// bitxor2 := bitand2 *(bitxor)
struct read_bitxor2 :
        seq< read_bitand2, star< read_bitxor > > {
};

// bitor := '|' bitxor2
struct read_bitor :
        read_bop< '|', read_bitxor2, bit_or< expression_type > > {
};
// bitor2 := bitxor2 *(bitor)
struct read_bitor2 :
        seq< read_bitxor2, star< read_bitor > > {
};

// land := '&&' bitor2
struct read_land :
        read_bop_m< pstring< '&', '&' >, read_bitor2, logical_and< expression_type > > {
};
// land2 := bitor2 *(land)
struct read_land2 :
        seq< read_bitor2, star< read_land > > {
};

// lor := '||' land2
struct read_lor :
        read_bop_m< pstring< '|', '|' >, read_land2, logical_or< expression_type > > {
};
// lor2 := land2 *(lor)
struct read_lor2 :
        seq< read_land2, star< read_lor > > {
};

// lor2 is the root of all expressions (as we don't consider even less bound ones)
// TODO: do we need to implement ternary operator? if we do, it will be right below lor2
struct read_expr :
        read_lor2 {
};

// mask := +(ALPHA) *(DIGIT)
struct mask :
        seq< plus< alpha >, star< digit > > {
};
// variable is just an identifier, can push it right away
struct push_mask :
        pad< ifapply< mask, mask_push >, space > {
};
// masks := mask *(',' mask)
struct masks :
        seq< push_mask, star< seq< chpad< ',' >, push_mask > > > {
};

template<int ...Name>
struct keyword :
        ifapply< strpad< Name... >, ask_keyword > {
};

struct requires_c :
        ifapply< ifmust< keyword< 'r', 'e', 'q', 'u', 'i', 'r', 'e', 's' >, read_expr >, ask_arguments > {
};

struct ensures_c :
        ifapply< ifmust< keyword< 'e', 'n', 's', 'u', 'r', 'e', 's' >, read_expr >, ask_arguments > {
};

struct assigns_c :
        ifapply< ifmust< keyword< 'a', 's', 's', 'i', 'g', 'n', 's' >, read_expr >, ask_arguments > {
};

struct assert_c :
        ifapply< ifmust< keyword< 'a', 's', 's', 'e', 'r', 't' >, read_expr >, ask_arguments > {
};

struct skip_c :
        ifapply< keyword< 's', 'k', 'i', 'p' >, ask_arguments > {
};
struct ignore_c :
        ifapply< keyword< 'i', 'g', 'n', 'o', 'r', 'e' >, ask_arguments > {
};
struct stack_depth_c :
        ifapply< ifmust< keyword< 's', 't', 'a', 'c', 'k', '-', 'd', 'e', 'p', 't', 'h' >, push_integer >, ask_arguments > {
};
struct mask_c :
        ifapply< ifmust< keyword< 'm', 'a', 's', 'k' >, masks >, ask_arguments > {
};
struct endmask_c :
        ifapply< keyword< 'e', 'n', 'd', 'm', 'a', 's', 'k' >, ask_arguments > {
};

struct commands :
        sor< requires_c, ensures_c, assigns_c, assert_c, skip_c, ignore_c, stack_depth_c, mask_c, endmask_c > {
};

struct acom :
		seq< one< '@' >, commands > {};

struct acom_c :
		ifapply< acom, store_command > {};
struct annotated_commentary :
	    sor< seq< pstring< '/', '/' >, plus< literal< acom_c > > >, seq< pstring< '/', '*' >, plus< literal< acom_c > >, pstring< '*', '/' > > > {};

struct read_calc :
        seq< commands, space_until_eof > {
};

template<class Location = pegtl::ascii_location>
vector< command_type > parse_command(const string& command) {

    unique_ptr < command_type > ret(new command_type());
    expr_stack stack;
    vector< command_type > commands;
    auto cp = anno::calc_parse_string< annotated_commentary, Location >(command, stack, *ret, commands);

    if(cp.success) {
    	return std::move(commands);
    }else {
//    	for(auto i = 0U; i < cp.str.size(); ++i) {
//    		std::cerr << i << "." << cp.str[i] << std::endl;
//    	}
    	return vector< command_type >();
    }
}

}   // namespace calculator
}   // namespace anno
}   // namespace borealis

#endif // ANNO_HPP
