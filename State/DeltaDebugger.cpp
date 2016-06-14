//
// Created by ice-phoenix on 4/29/15.
//

#include "SMT/Z3/ExecutionContext.h"
#include "SMT/Z3/ExprFactory.h"
#include "SMT/Z3/Logic.hpp"
#include "State/DeltaDebugger.h"
#include "State/Transformer/RandomSlicer.h"

namespace borealis {

DeltaDebugger::DeltaDebugger(ExprFactory& z3ef) : z3ef(z3ef) {}

PredicateState::Ptr DeltaDebugger::reduce(PredicateState::Ptr ps) {
    auto&& curr = ps;

    static config::ConfigEntry<int> dd_number("analysis", "dd-number");
    static config::ConfigEntry<int> dd_timeout("analysis", "dd-timeout");

    for (auto&& i = 0; i < dd_number.get(1000); ++i) {
        auto&& opt = RandomSlicer().transform(curr);
        dbgs() << "Trying from " << curr->size() << " to " << opt->size() << endl;

        auto&& s = tactics(dd_timeout.get(5) * 1000).mk_solver();

        ExecutionContext ctx(z3ef, memoryStart, memoryEnd);
        auto&& z3state = SMT<Z3>::doit(opt, z3ef, &ctx);

        s.add(z3state.asAxiom());

        if (z3::sat != s.check()) {
            if (opt->size() < curr->size()) {
                curr = opt;
            }
        }
    }

    return curr;
}

z3::tactic DeltaDebugger::tactics(unsigned int timeout) {
    auto&& c = z3ef.unwrap();

    auto&& smt_params = z3::params(c);
    smt_params.set("auto_config", true);
    smt_params.set("timeout", timeout);
    auto&& smt_tactic = with(z3::tactic(c, "smt"), smt_params);

    auto&& useful = z3::tactic(c, "ctx-simplify");

    return useful & smt_tactic;
}

} // namespace borealis
