/*
 * Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef BOOLECTOR_SOLVER_H_
#define BOOLECTOR_SOLVER_H_

#include "Logging/logger.hpp"
#include "SMT/Boolector/ExecutionContext.h"
#include "SMT/Boolector/ExprFactory.h"
#include "State/PredicateState.h"

#include "SMT/Result.h"

namespace borealis {
namespace boolector_ {

class Solver : public borealis::logging::ClassLevelLogging<Solver> {

    USING_SMT_LOGIC(Boolector);
    using ExprFactory = Boolector::ExprFactory;
    using ExecutionContext = Boolector::ExecutionContext;

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("boolector-solver")
#include "Util/unmacros.h"

    Solver(ExprFactory& bef, unsigned long long memoryStart, unsigned long long memoryEnd);

    smt::Result isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state);

    smt::Result isPathImpossible(
            PredicateState::Ptr path,
            PredicateState::Ptr state);

    void interrupt(); // here be dragons

private:

    ExprFactory& bef;

    unsigned long long memoryStart;
    unsigned long long memoryEnd;

    using check_result = std::tuple<int, boolectorpp::context*, util::option<int>, util::option<int>>;

    check_result check(
        const Bool& query,
        const Bool& state,
        const ExecutionContext& ctx);

};

} // namespace boolector_
} // namespace borealis

#endif // BOOLECTOR_SOLVER_H_
