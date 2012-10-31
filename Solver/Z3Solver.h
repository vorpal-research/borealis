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
#include "Query/Query.h"

namespace borealis {

class Z3Solver {

public:

    Z3Solver(Z3ExprFactory& z3ef);

    bool checkSat(
            const Query& q,
            const PredicateState& state);

    bool checkUnsat(
            const Query& q,
            const PredicateState& state);

    bool checkSatOrUnknown(
            const Query& q,
            const PredicateState& state);

    bool checkPathPredicates(
            const PredicateState& state);

private:

    Z3ExprFactory& z3ef;

    void addGEPAxioms(z3::solver& s);

    z3::check_result check(
            const Query& q,
            const PredicateState& state);

};

} // namespace borealis

#endif // Z3SOLVER_H_
