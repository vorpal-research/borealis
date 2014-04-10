/*
 * Divers.cpp
 *
 *  Created on: Apr 6, 2014
 *      Author: sam
 */

#include <algorithm>

#include <SMT/Z3/Divers.h>

namespace z3 {

std::vector<expr> diversify(solver& solver, const std::vector<expr>& divers) {
    unsigned int limit = std::max(32.0, pow(2, divers.size()));

    solver.push();
    auto models = diversify_unsafe(solver, divers, limit);
    solver.pop();

    return models;
}

std::vector<expr> diversify_unsafe(solver& solver, const std::vector<expr>& divers, unsigned limit) {
    std::vector<expr> models;
    models.reserve(limit);

    for (unsigned i = 0U; i < limit; ++i) {
        auto check = solver.check();

        if (check == unsat)
            break;

        auto valuation = solver.ctx().bool_val(true);

        auto model = solver.get_model();
        for (auto ex: divers) {
            auto val = model.eval(ex);
            valuation = valuation && (ex == val);
        }

        models.push_back(valuation);

        solver.add(not valuation);
    }

    return models;
}

} // namespace z3


