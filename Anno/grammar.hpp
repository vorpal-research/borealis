/*
 * grammar.hpp
 *
 *  Created on: Aug 15, 2013
 *      Author: belyaev
 */

#ifndef GRAMMAR_HPP_
#define GRAMMAR_HPP_

#include "Anno/calculator.hpp"

#include "Anno/zappo_macros.h"
#include "Util/macros.h"

#define PUSH(C) G(push<C>)
#define OR_G(...) pegtl::sor<__VA_ARGS__>
#define OR(...) G(OR_G(__VA_ARGS__))
#define SEQ_G(...) pegtl::seq<__VA_ARGS__>
#define SEQ(...) G(SEQ_G(__VA_ARGS__))

namespace borealis {
namespace anno {
namespace calculator {

using integer = GRAMMAR(
    CH('0') >> CH('x', 'X') >= +G(xdigit) | CH('0') >= *RANGE('0', '7') | RANGE('1', '9') >> *G(digit)
);
using push_integer = LITERALGRAMMAR( G(integer) & PUSH(long long) );
using boolean = GRAMMAR( S("true") | S("false") );
using push_boolean = LITERALGRAMMAR( G(boolean) & PUSH(bool) );
// FP grammar
// exponent := ('e'|'E') ('+'|'-') +(DIGIT)
using exponent = GRAMMAR( CH('e', 'E') >= CH('+', '-') >= +G(digit));
// floating := +(DIGIT) exponent                    // 1e+2
// floating := *(DIGIT) '.' +(DIGIT) ?exponent      // .23
// floating := +(DIGIT) '.' *(DIGIT) ?exponent      // 1.
using floating = GRAMMAR(
      +G(digit) >> G(exponent)                          /* 1e+2 */
    | *G(digit) >> CH('.') >> +G(digit) >> ~G(exponent) /* .23 */
    | +G(digit) >> CH('.') >> *G(digit) >> ~G(exponent) /* 1. */
);
// --//-- with action applied
using push_floating = LITERALGRAMMAR( G(floating) & PUSH(double) );
// variable is just an identifier, can push it right away
using push_variable = LITERALGRAMMAR( G(identifier) & PUSH(stdstring) );
// builtin is just an identifier prefixed with '\'
using push_builtin = LITERALGRAMMAR( (CH('\\') >> G(identifier)) & PUSH(stdstring) );
// a primitive is a variable or a literal
using push_primitive = OR_G (push_floating, push_integer, push_boolean, push_variable, push_builtin);

// a char surrounded by spaces
template<int C>
using chpad = LITERALGRAMMAR( CH(C) );

// a string surrounded by spaces
template<class PS>
using strpad = LITERALGRAMMAR( PSS(PS) );

template<class Grammar>
using literal = LITERALGRAMMAR( G(Grammar) );

// opening paren
using read_open = chpad< '(' >;
// closing paren
using read_close = chpad< ')' >;

// forward decl for the whole expression
struct read_expr;
// an atom is a primitive or a parentified expr
using  read_atom = GRAMMAR( G(push_primitive) | SEQ(read_open, read_expr, read_close));

// binary operation rule with oper sign being one char
// eq to:
//    bop := 'P' O with action A
template<int P, typename O, typename A>
struct read_bop :
        ifapply< seq< chpad< P >, O >, op_baction< A > > {};
// generic binary operation rule
// eq to:
//    bop := Rule O with action A
template<typename Rule, typename O, typename A>
struct read_bop_m;
template<typename O, typename A, int ...Chars>
struct read_bop_m< pegtl::string< Chars... >, O, A > :
        ifapply< seq< strpad< pegtl::string< Chars... > >, O >, op_baction< A > > {};

#define READ_BINARY(op, o, a) \
    read_bop_m< _PS(#op), o, a <expression_type> >

// unary operation rule
// eq to:
//    uop := 'P' O with action A
template<typename Rule, typename O, typename A>
struct read_uop_m;
template<typename O, typename A, int ...Chars>
struct read_uop_m< pegtl::string< Chars... >, O, A > :
        ifapply< seq< strpad< pegtl::string< Chars... > >, O >, op_uaction< A > > {};

#define READ_UNARY(op, o, a) \
    read_uop_m< _PS(#op), o, a <expression_type> >

// load := '*' atom
using read_load = READ_UNARY(*, read_atom, load);
// neg := '-' atom
using read_neg  = READ_UNARY(-, read_atom, negate);
// not := '!' atom
using read_not  = READ_UNARY(!, read_atom, logical_not);
// bnot := '~' atom
using read_bnot = READ_UNARY(~, read_atom, bit_not);
// an unary op is just an atom, neg, not or bnot
struct read_unary : OR_G(read_load, read_neg, read_not, read_bnot, read_atom) {};
// mul := '*' unary
using read_mul = READ_BINARY(*, read_unary, multiplies);
// div := '/' unary
using read_div = READ_BINARY(/, read_unary, divides);
// mod := '%' unary
using read_mod = READ_BINARY(%, read_unary, modulus);
// prod = unary *(mul | div | mod)
using read_prod = GRAMMAR( G(read_unary) >> *(OR(read_mul, read_div, read_mod)) );
// add := '+' prod
using read_add = READ_BINARY(+, read_prod, pluss);
// sub := '-' prod
using read_sub = READ_BINARY(-, read_prod, minus);
// sum := prod *(add | sub)
using read_sum = GRAMMAR( G(read_prod) >> *(G(read_add) | G(read_sub)) );
// lsh := '<<' sum
using read_lsh = READ_BINARY(<<, read_sum, left_shift);
// rsh := '>>' sum
using read_rsh = READ_BINARY(>>, read_sum, left_shift);
// shift := sum *(lsh | rsh)
using read_shift = GRAMMAR(G(read_sum) >> (*(G(read_lsh) | G(read_rsh))));
// gt := '>' shift
using read_gt = READ_BINARY(>, read_shift, greater);
// lt := '<' shift
using read_lt = READ_BINARY(<, read_shift, less);
// lt := '>=' shift
using read_ge = READ_BINARY(>=, read_shift, greater_or_equal);
// lt := '<=' shift
using read_le = READ_BINARY(<=, read_shift, less_or_equal);
// inequality := shift *(gt | lt | ge | le)
using read_inequality = GRAMMAR( G(read_shift) >> *(OR(read_ge, read_gt, read_le, read_lt)) );
// eq = '==' inequality
using read_eq = READ_BINARY(==, read_inequality, equal);
// neq = '!=' inequality
using read_neq = READ_BINARY(!=, read_inequality, nequal);
// equality := inequality *(eq | neq)
using read_equality = GRAMMAR( G(read_inequality) >> *(G(read_eq) | G(read_neq)) );
// bitand := '&' equality
using read_bitand = READ_BINARY(&, read_equality, bit_and);
// bitand2 := equality *(bitand)
using read_bitand2 = GRAMMAR(G(read_equality) >> *(G(read_bitand)));
// bitxor := '^' bitand2
using read_bitxor = READ_BINARY(^, read_bitand2, bit_xor);
// bitxor2 := bitand2 *(bitxor)
using read_bitxor2 = GRAMMAR(G(read_bitand2) >> *(G(read_bitxor)));
// bitor := '|' bitxor2
using read_bitor = READ_BINARY(|, read_bitxor2, bit_or);
// bitor2 := bitxor2 *(bitor)
using read_bitor2 = GRAMMAR( G(read_bitxor2) >> *G(read_bitor) );
// land := '&&' bitor2
using read_land = READ_BINARY(&&, read_bitor2, logical_and);
// land2 := bitor2 *(land)
using read_land2 = GRAMMAR( G(read_bitor2) >> *G(read_land) );
// lor := '||' land2
using read_lor = READ_BINARY(||, read_land2, logical_or);
// lor2 := land2 *(lor)
using read_lor2 = GRAMMAR( G(read_land2) >> *G(read_lor) );
// lor2 is the root of all expressions (as we don't consider even less bound ones)
// XXX: do we need to implement ternary operator (?:)
//      if we do, it will be right below lor2
struct read_expr :
        read_lor2 {};
using read_expr_list = GRAMMAR( G(read_expr) >> *( CH(',') >> G(read_expr) ) );
using read_call = GRAMMAR( G(push_builtin) >> CH('(') >> G(read_expr_list) >> CH(')') );

#undef READ_UNARY
#undef READ_BINARY

// mask := +(ALPHA) *(DIGIT)
using mask = GRAMMAR( +(G(alpha)) >> *(G(digit)) );
// mask is just an identifier, can push it right away
using push_mask = LITERALGRAMMAR( G(mask) & G(mask_push) );
// masks := mask *(',' mask)
using masks = GRAMMAR(G(push_mask) >> *(G(chpad<','>) >> G(push_mask)));

template<class PS>
struct keyword :
        ifapply< strpad< PS >, ask_keyword > {};

#define KEYWORD(STR) keyword<_PS(STR)>
struct requires_c :
        ifapply< ifmust< KEYWORD("requires"), read_expr >, ask_arguments > {};
struct ensures_c :
        ifapply< ifmust< KEYWORD("ensures"), read_expr >, ask_arguments > {};
struct assigns_c :
        ifapply< ifmust< KEYWORD("assigns"), read_expr >, ask_arguments > {};
struct assert_c :
        ifapply< ifmust< KEYWORD("assert"), read_expr >, ask_arguments > {};
struct assume_c :
        ifapply< ifmust< KEYWORD("assume"), read_expr >, ask_arguments > {};
struct skip_c :
        ifapply< KEYWORD("skip"), ask_arguments > {};
struct ignore_c :
        ifapply< KEYWORD("ignore"), ask_arguments > {};
struct inline_c :
        ifapply< KEYWORD("inline"), ask_arguments > {};
struct stack_depth_c :
        ifapply< ifmust< KEYWORD("stack-depth"), push_integer >, ask_arguments > {};
struct unroll_c :
        ifapply< ifmust< KEYWORD("unroll"), push_integer >, ask_arguments > {};
struct mask_c :
        ifapply< ifmust< KEYWORD("mask"), masks >, ask_arguments > {};
struct endmask_c :
        ifapply< KEYWORD("endmask"), ask_arguments > {};
#undef KEYWORD

struct commands :
        sor<
            requires_c,
            ensures_c,
            assigns_c,
            assume_c,
            assert_c,
            skip_c,
            ignore_c,
            inline_c,
            stack_depth_c,
            unroll_c,
            mask_c,
            endmask_c
        > {};

struct acom :
        seq< one< '@' >, commands > {};
struct acom_c :
        ifapply< acom, store_command > {};
struct annotated_commentary :
        sor< seq< _PS("//"), plus< literal< acom_c > > >,
             seq< _PS("/*"), plus< literal< acom_c > >, _PS("*/") > > {};

struct read_calc :
        seq< commands, space_until_eof > {};

} // namespace calculator
} // namespace anno
} // namespace borealis

#undef SEQ
#undef SEQ_G
#undef OR
#undef OR_G
#undef PUSH

#include "Util/unmacros.h"
#include "Anno/zappo_unmacros.h"

#endif /* GRAMMAR_HPP_ */
