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

#include "SMT/Result.h"

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

    Solver(ExprFactory& msatef, unsigned long long memoryStart, unsigned long long memoryEnd);

    smt::Result isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    smt::Result isPathImpossible(
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
    unsigned long long memoryStart;
    unsigned long long memoryEnd;

    using check_result = std::tuple<
        msat_result,
        util::option<mathsat::Model>,
        util::option<std::vector<mathsat::Expr>>,
        util::option<std::string> // FIXME: MathSAT proof
    >;

    check_result check(
            const Bool& msatquery_,
            const Bool& msatstate_);

    Bool probeZ3(PredicateState::Ptr body,
                 PredicateState::Ptr query,
                 const std::vector<Term::Ptr>& args,
                 const std::vector<Term::Ptr>& pathVars);

};

} // namespace mathsat_
} // namespace borealis

#endif /* MATHSAT_SOLVER_H_ */
