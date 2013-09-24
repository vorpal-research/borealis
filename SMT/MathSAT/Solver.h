/*
 * Solver.h
 *
 *  Created on: Aug 5, 2013
 *      Author: sam
 */

#ifndef MATHSAT_SOLVER_H_
#define MATHSAT_SOLVER_H_

#include "Logging/logger.hpp"
#include "SMT/MathSAT/ExecutionContext.h"
#include "SMT/MathSAT/ExprFactory.h"
#include "State/PredicateState.h"

namespace borealis {
namespace mathsat_ {

class Solver : public borealis::logging::ClassLevelLogging<Solver> {

    USING_SMT_LOGIC(MathSAT);

    typedef MathSAT::ExprFactory ExprFactory;
    typedef MathSAT::ExecutionContext ExecutionContext;

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("mathsat-solver")
#include "Util/unmacros.h"

    Solver(ExprFactory& msatef);

    bool isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    bool isPathImpossible(
            PredicateState::Ptr path,
            PredicateState::Ptr state);

    Dynamic getInterpolant(
            PredicateState::Ptr query,
            PredicateState::Ptr body);

    Dynamic getSummary(
            const std::vector<Term::Ptr>& args,
            PredicateState::Ptr query,
            PredicateState::Ptr body);

    Dynamic getContract(
                const std::vector<Term::Ptr>& args,
                PredicateState::Ptr query,
                PredicateState::Ptr body);

private:

    ExprFactory& msatef;

    msat_result check(
            const Bool& msatquery,
            const Bool& msatstate);

};

} // namespace mathsat_
} // namespace borealis

#endif /* MATHSAT_SOLVER_H_ */
