
#include <stp/c_interface.h>

#include "SMT/STP/Unlogic/Unlogic.h"

#include "Util/macros.h"

namespace borealis {
namespace stp_ {
namespace unlogic {

Term::Ptr undoThat(const FactoryNest& FN, Term::Ptr witness, borealis::STP::Dynamic dyn) {
    auto&& ctx = dyn.getCtx();
    auto&& assign_ = vc_getCounterExample(ctx, dyn.getExpr());

    if(dyn.getSort().is_bool()) {
        return FN.Term->getBooleanTerm(static_cast<bool>(vc_isBool(assign_)));
    }

    auto bwidth = dyn.getSort().bv_size();
    if(bwidth <= sizeof(unsigned long long) * 8) {
        auto ull = getBVUnsignedLongLong(assign_);
        auto ll = static_cast<int64_t>(ull);
        return FN.Term->getIntTerm(ll, witness->getType());
    }

    auto assign = exprString(assign_);
    ON_SCOPE_EXIT(free(assign));
    llvm::StringRef sr(assign);

    ASSERTC(sr.startswith("0x"));
    sr = sr.drop_front(2);

    llvm::APInt parsed;
    sr.getAsInteger(0x10, parsed);

    return FN.Term->getIntTerm(parsed)->setType(FN.Term.get(), witness->getType());
}

} /* namespace unlogic */
} /* namespace stp_ */
} /* namespace borealis */

#include "Util/unmacros.h"
