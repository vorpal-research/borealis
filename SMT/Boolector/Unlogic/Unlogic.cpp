#include "SMT/Boolector/Unlogic/Unlogic.h"

namespace borealis {
namespace boolector_ {
namespace unlogic {

Term::Ptr undoThat(const FactoryNest& FN, Term::Ptr witness, borealis::Boolector::Dynamic dyn) {
    auto&& btor = dyn.getCtx();

    auto&& assign_ = boolector_bv_assignment(btor, dyn.getExpr());
    std::string assign = assign_;
    boolector_free_bv_assignment(btor, assign_);

    for(auto&& ch: assign) {
        if(ch == 'X') ch = '0';
    }

    llvm::APInt parsed(dyn.getSort().bv_size(), assign, 2);

    if(dyn.getSort().is_bool()) {
        return FN.Term->getBooleanTerm(!!parsed);
    }

    return FN.Term->getIntTerm(parsed)->setType(witness->getType());
}

} /* namespace unlogic */
} /* namespace boolector_ */
} /* namespace borealis */

