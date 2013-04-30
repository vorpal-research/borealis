/*
 * Z3Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3SOLVER_H_
#define Z3SOLVER_H_

#include "Logging/logger.hpp"
#include "Solver/Z3ExprFactory.h"
#include "State/PredicateState.h"

namespace borealis {

class Z3Solver : public borealis::logging::ClassLevelLogging<Z3Solver> {

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("z3solver")
#include "Util/unmacros.h"

    Z3Solver(Z3ExprFactory& z3ef);

    bool checkViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    bool checkPathPredicates(
            PredicateState::Ptr path,
            PredicateState::Ptr state);

private:

    Z3ExprFactory& z3ef;

    z3::check_result check(
            const logic::Bool& z3query,
            const logic::Bool& z3state);

};

} // namespace borealis

#endif // Z3SOLVER_H_
