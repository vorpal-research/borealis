/*
 * Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef PORTFOLIO_SOLVER_H_
#define PORTFOLIO_SOLVER_H_

#include "Logging/logger.hpp"
#include "SMT/Z3/ExecutionContext.h"
#include "SMT/Z3/ExprFactory.h"
#include "State/PredicateState.h"

#include "SMT/Result.h"

namespace borealis {
namespace portfolio_smt_ {

class Solver : public borealis::logging::ClassLevelLogging<Solver> {

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("portfolio-solver")
#include "Util/unmacros.h"

    Solver(unsigned long long memoryStart, unsigned long long memoryEnd);

    smt::Result isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state) {



    }

    smt::Result isPathImpossible(
            PredicateState::Ptr path,
            PredicateState::Ptr state) {

    }


};

} // namespace portfolio_smt_
} // namespace borealis

#endif // PORTFOLIO_SOLVER_H_
