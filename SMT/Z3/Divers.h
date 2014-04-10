/*
 * Divers.h
 *
 *  Created on: Apr 6, 2014
 *      Author: sam
 */

#ifndef Z3_DIVERS_H_
#define Z3_DIVERS_H_

#include <z3/z3++.h>

#include <vector>

namespace z3 {

std::vector<expr> diversify(solver& solver, const std::vector<expr>& divers);

std::vector<expr> diversify_unsafe(solver& solver, const std::vector<expr>& divers, unsigned limit);

} // namespace z3

#endif /* Z3_DIVERS_H_ */
