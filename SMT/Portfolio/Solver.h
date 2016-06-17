/*
 * Solver.h
 *
 *  Created on: Oct 31, 2012
 *      Author: ice-phoenix
 */

#ifndef PORTFOLIO_SOLVER_H_
#define PORTFOLIO_SOLVER_H_

#include <condition_variable>
#include <future>

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

    Solver(unsigned long long memoryStart, unsigned long long memoryEnd):
        memoryStart(memoryStart), memoryEnd(memoryEnd) {}

    smt::Result isViolated(
            PredicateState::Ptr query,
            PredicateState::Ptr state) {

        Z3::ExprFactory z3ef;
        Z3::Solver z3s (z3ef. memoryStart, memoryEnd);

        CVC4::ExprFactory cvc4ef;
        CVC4::Solver cvc4s (cvc4ef. memoryStart, memoryEnd);

        std::condition_variable cv;

        auto run = [=](auto&& solver, auto&& query, auto&& state) {
            auto ret = solver.isViolated(query, state);
            cv.notify_all();
            return ret;
        };

        auto z3res = std::async(std::launch::async, run, std::ref(z3s), query, state);
        auto cvc4res = std::async(std::launch::async, run, std::ref(cvc4s), query, state);

        smt::Result res;

        std::mutex wha;
        std::unique_lock lock(wha);
        while(!z3res.valid() && !cvc4res.valid())
            cv.wait(lock);

        if(z3res.valid()) {
            res = z3res.get();

            cvc4s.interrupt();
            cvc4res.wait();
        } else if(cvc4res.valid()) {
            res = cvc4res.get();

            z3s.interrupt();
            z3res.wait();
        }

        return res;
    }

};

} // namespace portfolio_smt_
} // namespace borealis

#endif // PORTFOLIO_SOLVER_H_
