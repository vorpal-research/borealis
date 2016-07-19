//
// Created by belyaev on 4/6/15.
//

#include "SMT/Result.h"
#include "Term/OpaqueBoolConstantTerm.h"
#include "Term/OpaqueIntConstantTerm.h"

#include "Util/macros.h"

namespace borealis {
namespace smt {

SatResult::SatResult() :
    model(nullptr) {}

std::ostream& operator<<(std::ostream& ost, const SatResult& res) {
    ost << "sat";
    if(res.model) {
        ost << "[" << res.getModel() << "]";
    }
    return ost;
}

} // namespace smt
} // namespace borealis

#include "Util/unmacros.h"
