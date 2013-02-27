/*
 * Z3Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3SOLVER_H_
#define Z3SOLVER_H_

#include "Solver/Z3ExprFactory.h"
#include "State/PredicateState.h"

namespace borealis {

class Z3Solver {

public:

    Z3Solver(Z3ExprFactory& z3ef);

    bool checkViolated(
            const PredicateState& query,
            const PredicateState& state);

    bool checkPathPredicates(
            const PredicateState& path,
            const PredicateState& state);

private:

    Z3ExprFactory& z3ef;

    z3::check_result check(
            const logic::Bool& z3query,
            const logic::Bool& z3state);

};

} // namespace borealis

#endif // Z3SOLVER_H_
