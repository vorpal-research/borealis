/*
 * util.cpp
 *
 *  Created on: Oct 12, 2012
 *      Author: ice-phoenix
 */

#include "util.h"

namespace borealis {

z3::check_result check(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    using namespace::z3;

    solver s(ctx);

    for(auto& ss : state) {
        s.add(ss);
    }

    expr assertionPredicate = ctx.bool_const("$isTrue$");
    s.add(implies(assertionPredicate, assertion));

    return s.check(1, &assertionPredicate);
}

bool checkSat(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    return check(assertion, state, ctx) == z3::sat;
}

bool checkUnsat(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    return check(assertion, state, ctx) == z3::unsat;
}

bool checkSatOrUnknown(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx) {
    return check(assertion, state, ctx) != z3::unsat;
}

} // namespace borealis
