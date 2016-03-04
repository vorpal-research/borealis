/*
 * Divers.cpp
 *
 *  Created on: Apr 6, 2014
 *      Author: sam
 */

#include <algorithm>

#include "Logging/logger.hpp"
#include "SMT/Z3/Divers.h"

using namespace borealis::logging;

namespace z3 {

std::vector<model> diversify(solver& solver,
                             const std::vector<expr>& divers) {
    unsigned int limit = std::max(32.0, pow(2, divers.size()));

    solver.push();
    auto models = diversify_unsafe(solver, divers, limit);
    solver.pop();

    return models;
}


std::vector<model> diversify_unsafe(solver& solver,
                                    const std::vector<expr>& divers,
                                    unsigned limit) {
    std::vector<model> models;
    models.reserve(limit);

    for (unsigned i = 0U; i < limit; ++i) {
        auto check = solver.check();

        if (check == unsat) break;

        if (check == unknown) {
            wtf() << "Encountered z3::unknown when diversifying stuff..." << endl;
            continue;
        }

        auto block = solver.ctx().bool_val(true);

        auto model = solver.get_model();
        for (auto ex : divers) {
            auto val = model.eval(ex);
            block = block && (ex == val);
        }

        models.push_back(model);

        solver.add(not block);
    }

    return models;
}

std::vector<expr> collect_models(const solver& solver,
                                 const std::vector<expr>& collect,
                                 const std::vector<model>& models) {
    std::vector<expr> collected;
    collected.reserve(models.size());

    for (auto model : models) {
        auto valuation = solver.ctx().bool_val(true);

        for (auto ex : collect) {
            auto val = model.eval(ex);
            valuation = valuation && (ex == val);
        }
        collected.push_back(valuation);
    }
    return collected;
}

std::vector<expr> diversify(solver& solver,
                            const std::vector<expr>& divers,
                            const std::vector<expr>& collect) {
    auto models = diversify(solver, divers);
    return collect_models(solver, collect, models);
}

std::vector<expr> diversify_unsafe(solver& solver,
                                   const std::vector<expr>& divers,
                                   const std::vector<expr>& collect,
                                   unsigned limit) {
    auto models = diversify_unsafe(solver, divers, limit);
    return collect_models(solver, collect, models);
}

} // namespace z3
