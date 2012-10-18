/*
 * util.h
 *
 *  Created on: Oct 12, 2012
 *      Author: ice-phoenix
 */

#ifndef SOLVER_UTIL_H_
#define SOLVER_UTIL_H_

#include <z3/z3++.h>

#include <vector>

namespace borealis {

bool checkSat(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx);

bool checkUnsat(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx);

bool checkSatOrUnknown(
        const z3::expr& assertion,
        const std::vector<z3::expr>& state,
        z3::context& ctx);

} // namespace borealis

#endif /* SOLVER_UTIL_H_ */
