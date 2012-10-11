/*
 * Z3Pass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: ice-phoenix
 */

#include "Z3Pass.h"

#include <z3/z3++.h>

namespace borealis {

using util::streams::endl;
using util::toString;

typedef PredicateState PS;
typedef PredicateStateAnalysis::PredicateStateMapEntry PSME;

Z3Pass::Z3Pass() : llvm::FunctionPass(ID) {
    // TODO
}

void Z3Pass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
    using namespace::llvm;

    Info.setPreservesAll();
    Info.addRequiredTransitive<PredicateStateAnalysis>();
}

bool Z3Pass::runOnFunction(llvm::Function& F) {
    using namespace::std;
    using namespace::llvm;
    using namespace::z3;

    PSA = &getAnalysis<PredicateStateAnalysis>();

    for_each(PSA->getPredicateStateMap(), [this](const PSME& psme) {
        auto psv = psme.second;
        for(auto& ps : psv) {
            context ctx;
            solver s(ctx);

            auto z3 = ps.toZ3(ctx);
            auto& pathPredicate = z3.first;
            auto& statePredicate = z3.second;
            s.add(statePredicate);

            errs() << ps << endl;
            errs() << "Path predicates:" << endl <<
                    toString(pathPredicate) << endl;
            errs() << "State predicates:" << endl <<
                    toString(statePredicate) << endl;

            expr reachabilityPredicate = ctx.bool_const("$isReachable$");
            s.add(implies(reachabilityPredicate, pathPredicate));

            bool isReachable = s.check(1, &reachabilityPredicate);
            if (isReachable) {
                errs() << "State is reachable" << endl;
            } else {
                errs() << "State is unreachable" << endl;
            }
        }
    });

    return false;
}

Z3Pass::~Z3Pass() {
    // TODO
}

} /* namespace borealis */

char borealis::Z3Pass::ID;
static llvm::RegisterPass<borealis::Z3Pass>
X("z3", "Predicate SMT analysis via Z3", false, false);
