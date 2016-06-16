/*
 * Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef CVC4_SOLVER_H_
#define CVC4_SOLVER_H_

#include "Logging/logger.hpp"
#include "SMT/CVC4/ExecutionContext.h"
#include "SMT/CVC4/ExprFactory.h"
#include "State/PredicateState.h"

#include "SMT/Result.h"

namespace borealis {
namespace cvc4_ {

class Solver : public borealis::logging::ClassLevelLogging<Solver> {

    USING_SMT_LOGIC(CVC4);
    using ExprFactory = CVC4::ExprFactory;
    using ExecutionContext = CVC4::ExecutionContext;

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("cvc4-solver")
#include "Util/unmacros.h"

    Solver(ExprFactory& cvc4ef, unsigned long long memoryStart, unsigned long long memoryEnd);

    smt::Result isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    smt::Result isPathImpossible(
            PredicateState::Ptr path,
            PredicateState::Ptr state);

private:

    ExprFactory& cvc4ef;
    unsigned long long memoryStart;
    unsigned long long memoryEnd;

    using check_result = std::tuple<::CVC4::Result, std::shared_ptr<::CVC4::SmtEngine>, util::option<int>, util::option<int>>;

    check_result check(
        const Bool& query,
        const Bool& state,
        const ExecutionContext& ctx);

};

} // namespace cvc4_
} // namespace borealis

#endif // CVC4_SOLVER_H_
