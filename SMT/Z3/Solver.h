/*
 * Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef Z3_SOLVER_H_
#define Z3_SOLVER_H_

#include "Logging/logger.hpp"
#include "SMT/Z3/ExecutionContext.h"
#include "SMT/Z3/ExprFactory.h"
#include "State/PredicateState.h"

namespace borealis {
namespace z3_ {

class Solver : public borealis::logging::ClassLevelLogging<Solver> {

    USING_SMT_LOGIC(Z3);
    typedef Z3::ExprFactory ExprFactory;
    typedef Z3::ExecutionContext ExecutionContext;

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("z3-solver")
#include "Util/unmacros.h"

    Solver(ExprFactory& z3ef, unsigned long long memoryStart, unsigned long long memoryEnd);

    bool isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    bool isPathImpossible(
            PredicateState::Ptr path,
            PredicateState::Ptr state);

    PredicateState::Ptr probeModels(
            PredicateState::Ptr body,
            PredicateState::Ptr query,
            const std::vector<Term::Ptr>& diversifiers,
            const std::vector<Term::Ptr>& collectibles);

private:

    ExprFactory& z3ef;
    unsigned long long memoryStart;
    unsigned long long memoryEnd;

    using check_result = std::tuple<
        z3::check_result,
        util::option<z3::model>,
        util::option<z3::expr_vector>,
        util::option<std::string>
    >;

    check_result check(
            const Bool& z3query,
            const Bool& z3state);

    z3::tactic tactics();

};

} // namespace z3_
} // namespace borealis

#endif // Z3_SOLVER_H_
