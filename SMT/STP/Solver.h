/*
 * Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef STP_SOLVER_H_
#define STP_SOLVER_H_

#include "Logging/logger.hpp"
#include "SMT/STP/ExecutionContext.h"
#include "SMT/STP/ExprFactory.h"
#include "State/PredicateState.h"

#include "SMT/Result.h"

namespace borealis {
namespace stp_ {

class Solver : public borealis::logging::ClassLevelLogging<Solver> {

    USING_SMT_LOGIC(STP);
    using ExprFactory = STP::ExprFactory;
    using ExecutionContext = STP::ExecutionContext;

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("stp-solver")
#include "Util/unmacros.h"

    Solver(ExprFactory& bef, unsigned long long memoryStart, unsigned long long memoryEnd);

    smt::Result isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    void interrupt(); // here be dragons

private:

    ExprFactory& bef;

    unsigned long long memoryStart;
    unsigned long long memoryEnd;

    using check_result = std::tuple<int, stppp::context*, util::option<int>, util::option<int>>;

    check_result check(
        const Bool& query,
        const Bool& state,
        const ExecutionContext& ctx);

};

} // namespace stp_
} // namespace borealis

#endif // STP_SOLVER_H_
