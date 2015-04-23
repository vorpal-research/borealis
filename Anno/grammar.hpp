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

// integer := ('0' ('x'|'X') XDIGIT+) | ('0' ({'0'-'7'})*) | ( {'1'-'9'} DIGIT*)
using integer = GRAMMAR(
    CH('0') >> CH('x', 'X') >= +G(xdigit) | CH('0') >= *RANGE('0', '7') | RANGE('1', '9') >> *G(digit)
);
using push_integer = LITERALGRAMMAR( G(integer) & PUSH(long long) );
// boolean := "true" | "false"
struct boolean : SGRAMMAR(boolean, S("true") | S("false") );
using push_boolean = LITERALGRAMMAR( G(boolean) & PUSH(bool) );
// FP grammar
// exponent := ('e'|'E') ('+'|'-') (DIGIT)+
using exponent = GRAMMAR( CH('e', 'E') >= CH('+', '-') >= +G(digit));
// floating := (DIGIT)+ exponent                    // 1e+2
// floating := (DIGIT)* '.' (DIGIT)+ ?exponent      // .23
// floating := (DIGIT)+ '.' (DIGIT)* ?exponent      // 1.
struct floating: SGRAMMAR(floating,
      +G(digit) >> G(exponent)                          /* 1e+2 */
    | *G(digit) >> CH('.') >> +G(digit) >> ~G(exponent) /* .23 */
    | +G(digit) >> CH('.') >> *G(digit) >> ~G(exponent) /* 1. */
);
// --//-- with action applied
using push_floating = LITERALGRAMMAR( G(floating) & PUSH(double) );
// variable := IDENTIFIER
using push_variable = LITERALGRAMMAR( G(identifier) & PUSH(stdstring) );
// builtin := '\' IDENTIFIER
using push_builtin = LITERALGRAMMAR( (CH('\\') >> G(identifier)) & PUSH(stdstring) );
// note the boolean is before variable, as e.g. 'true' is a valid variable name
// primitive := floating | integer | boolean | variable | builtin
using push_primitive = OR_G (push_floating, push_integer, push_boolean, push_variable, push_builtin);

// helpers -----------------------------------------------------------------------------------------
// a char surrounded by spaces
template<int C>
using chpad = LITERALGRAMMAR( CH(C) );

// a string surrounded by spaces
template<class PS>
using strpad = LITERALGRAMMAR( PSS(PS) );

template<class Grammar>
using literal = LITERALGRAMMAR( G(Grammar) );
// helpers end -------------------------------------------------------------------------------------


// opening paren
using read_open = chpad< '(' >;
using read_sq_open = chpad< '[' >;
// closing paren
using read_close = chpad< ')' >;
using read_sq_close = chpad< ']' >;
using read_comma = chpad< ',' >;
using read_dot = chpad< '.' >;
using read_arrow = LITERALGRAMMAR( S("->") );

// forward decl for the whole expression
struct read_expr;
// an atom is a primitive or a parentified expr
// atom := primitive | '(' expr ')'
struct read_atom : SGRAMMAR(read_atom, G(push_primitive) | SEQ(read_open, read_expr, read_close));
// expr_list := expr (',' expr)*
struct read_expr_list : SGRAMMAR(read_expr_list,
    (G(read_comma) >= G(read_expr)) & G(op_collect_list)
);
struct read_expr_list2: SGRAMMAR(read_expr_list2,
    G(read_expr) >> *G(read_expr_list)
);
// property_expr := '.' variable
struct read_property_expr: SGRAMMAR(read_property_expr,
    (G(read_dot) >> G(push_variable)) & G(op_baction<dots<expression_type>>)
);
// indirect_propery_expr := '->' variable
struct read_indirect_property_expr: SGRAMMAR(read_indirect_property_expr,
    (G(read_arrow) >> G(push_variable)) & G(op_baction<arrows<expression_type>>)
);
// index_expr := '[' expr ']'
struct read_index_expr: SGRAMMAR(read_index_expr,
    (G(read_sq_open) >= G(read_expr) >= G(read_sq_close)) & G(op_baction<indices<expression_type>>)
);
// calling_expr := '(' expr ')'
struct read_calling_expr: SGRAMMAR(read_calling_expr,
    (G(read_open) >= G(read_expr_list2) >= G(read_close)) & G(op_baction<calls<expression_type>>)
);
// postfix_expr := atom (index_expr | calling_expr | property_expr | indirect_property_expr)*
struct read_postfix_expr : SGRAMMAR(read_postfix_expr,
    G(read_atom) >> *(G(read_index_expr) | G(read_calling_expr) | G(read_property_expr) | G(read_indirect_property_expr))
);

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
    read_bop_m< _PS(#op), o, a <expression_type> > {};

// unary operation rule
// eq to:
//    uop := 'P' O with action A
template<typename Rule, typename O, typename A>
struct read_uop_m;
template<typename O, typename A, int ...Chars>
struct read_uop_m< pegtl::string< Chars... >, O, A > :
        ifapply< seq< strpad< pegtl::string< Chars... > >, O >, op_uaction< A > > {};

#define READ_UNARY(op, o, a) \
    read_uop_m< _PS(#op), o, a <expression_type> > {};

// load := '*' postfix_expr
struct read_load: READ_UNARY(*, read_postfix_expr, load);
// neg := '-' postfix_expr
struct read_neg: READ_UNARY(-, read_postfix_expr, negate);
// not := '!' postfix_expr
struct read_not: READ_UNARY(!, read_postfix_expr, logical_not);
// bnot := '~' postfix_expr
struct read_bnot: READ_UNARY(~, read_postfix_expr, bit_not);
// unary := load | neg | not | bnot | postfix_expr
struct read_unary: OR_G(read_load, read_neg, read_not, read_bnot, read_postfix_expr) {};
// mul := '*' unary
struct read_mul: READ_BINARY(*, read_unary, multiplies);
// div := '/' unary
struct read_div: READ_BINARY(/, read_unary, divides);
// mod := '%' unary
struct read_mod: READ_BINARY(%, read_unary, modulus);
// prod = unary (mul | div | mod)*
struct read_prod: SGRAMMAR(read_prod, G(read_unary) >> *(OR(read_mul, read_div, read_mod)) );
// add := '+' prod
struct read_add: READ_BINARY(+, read_prod, pluss);
// sub := '-' prod
struct read_sub: READ_BINARY(-, read_prod, minus);
// sum := prod (add | sub)*
struct read_sum: SGRAMMAR(read_sum, G(read_prod) >> *(G(read_add) | G(read_sub)) );
// lsh := '<<' sum
struct read_lsh: READ_BINARY(<<, read_sum, left_shift);
// rsh := '>>' sum
struct read_rsh: READ_BINARY(>>, read_sum, left_shift);
// shift := sum (lsh | rsh)*
struct read_shift: SGRAMMAR(read_shift, G(read_sum) >> (*(G(read_lsh) | G(read_rsh))));
// gt := '>' shift
struct read_gt: READ_BINARY(>, read_shift, greater);
// lt := '<' shift
struct read_lt: READ_BINARY(<, read_shift, less);
// lt := '>=' shift
struct read_ge: READ_BINARY(>=, read_shift, greater_or_equal);
// lt := '<=' shift
struct read_le: READ_BINARY(<=, read_shift, less_or_equal);
// inequality := shift (gt | lt | ge | le)*
struct read_inequality: SGRAMMAR(read_inequality, G(read_shift) >> *(OR(read_ge, read_gt, read_le, read_lt)) );
// eq = '==' inequality
struct read_eq: READ_BINARY(==, read_inequality, equal);
// neq = '!=' inequality
struct read_neq: READ_BINARY(!=, read_inequality, nequal);
// equality := inequality (eq | neq)*
struct read_equality: SGRAMMAR(read_equality, G(read_inequality) >> *(G(read_eq) | G(read_neq)) );
// bitand := '&' equality
struct read_bitand: READ_BINARY(&, read_equality, bit_and);
// bitand2 := equality bitand*
struct read_bitand2: SGRAMMAR(read_bitand2, G(read_equality) >> *(G(read_bitand)));
// bitxor := '^' bitand2
struct read_bitxor: READ_BINARY(^, read_bitand2, bit_xor);
// bitxor2 := bitand2 bitxor*
struct read_bitxor2: SGRAMMAR(read_bitxor2, G(read_bitand2) >> *(G(read_bitxor)));
// bitor := '|' bitxor2
struct read_bitor: READ_BINARY(|, read_bitxor2, bit_or);
// bitor2 := bitxor2 bitor*
struct read_bitor2: SGRAMMAR(read_bitor2, G(read_bitxor2) >> *G(read_bitor) );
// land := '&&' bitor2
struct read_land: READ_BINARY(&&, read_bitor2, logical_and);
// land2 := bitor2 land*
struct read_land2: SGRAMMAR(read_land2, G(read_bitor2) >> *G(read_land) );
// lor := '||' land2
struct read_lor: READ_BINARY(||, read_land2, logical_or);
// lor2 := land2 lor*
struct read_lor2: SGRAMMAR(read_lor2, G(read_land2) >> *G(read_lor) );
// implies := '==>' lor2
struct read_implies: READ_BINARY(==>, read_lor2, implies);
// implies2 := lor2 implies*
struct read_implies2: SGRAMMAR(read_implies2, G(read_lor2) >> *G(read_implies) );
// lor2 is the root of all expressions (as we don't consider even less bound ones)
// XXX: do we need to implement ternary operator (?:)
//      if we do, it will be right below lor2
// expr := lor2
struct read_expr :
        read_implies2 {};

#undef READ_UNARY
#undef READ_BINARY

// mask := (ALPHA)+ (DIGIT)*
using mask = GRAMMAR( +(G(alpha)) >> *(G(digit)) );
// mask is just an identifier, can push it right away
using push_mask = LITERALGRAMMAR( G(mask) & G(mask_push) );
// masks := mask (',' mask)*
using masks = GRAMMAR(G(push_mask) >> *(G(chpad<','>) >> G(push_mask)));

template<class PS>
struct keyword :
        ifapply< strpad< PS >, ask_keyword > {};

#define KEYWORD(STR) keyword<_PS(STR)>
// requires := "requires" expr
struct requires_c :
        ifapply< ifmust< KEYWORD("requires"), read_expr >, ask_arguments > {};
// ensures := "ensures" expr
struct ensures_c :
        ifapply< ifmust< KEYWORD("ensures"), read_expr >, ask_arguments > {};
// assigns := "assigns" expr
struct assigns_c :
        ifapply< ifmust< KEYWORD("assigns"), read_expr >, ask_arguments > {};
// assert := "assert" expr
struct assert_c :
        ifapply< ifmust< KEYWORD("assert"), read_expr >, ask_arguments > {};
// assume := "assume" expr
struct assume_c :
        ifapply< ifmust< KEYWORD("assume"), read_expr >, ask_arguments > {};
// skip := "skip"
struct skip_c :
        ifapply< KEYWORD("skip"), ask_arguments > {};
// ignore := "ignore"
struct ignore_c :
        ifapply< KEYWORD("ignore"), ask_arguments > {};
// inline := "inline"
struct inline_c :
        ifapply< KEYWORD("inline"), ask_arguments > {};
// stack-depth := "stack-depth" integer
struct stack_depth_c :
        ifapply< ifmust< KEYWORD("stack-depth"), push_integer >, ask_arguments > {};
// unroll := "unroll" integer
struct unroll_c :
        ifapply< ifmust< KEYWORD("unroll"), push_integer >, ask_arguments > {};
// maska := "mask" masks
struct mask_c :
        ifapply< ifmust< KEYWORD("mask"), masks >, ask_arguments > {};
// endmask := "endmask"
struct endmask_c :
        ifapply< KEYWORD("endmask"), ask_arguments > {};
#undef KEYWORD

// commands := requires | ensures | assigns | assume | assert | skip
//           | ignore | inline | stack-depth | unroll | mask | endmask
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

// acom := '@' commands
struct acom :
        seq< one< '@' >, commands > {};
struct acom_c :
        ifapply< acom, store_command > {};
// annotated_commentary := ("//" acom+) | ("/*" acom+ "*/")
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
