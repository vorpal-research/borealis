//
// Created by belyaev on 6/30/16.
//

#ifndef PORTFOLIOSOLVER_H
#define PORTFOLIOSOLVER_H

#include <memory>

#include "Logging/logger.hpp"
#include "SMT/Result.h"
#include "State/PredicateState.h"

namespace borealis {
namespace portfolio_ {

class Solver: public borealis::logging::ClassLevelLogging<Solver>  {

    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("portfolio-solver")
#include "Util/unmacros.h"

    Solver(unsigned long long memoryStart, unsigned long long memoryEnd);
    ~Solver();
    smt::Result isViolated(PredicateState::Ptr query, PredicateState::Ptr state);
};

} /* namespace portfolio_ */
} /* namespace borealis */

#endif // PORTFOLIOSOLVER_H
