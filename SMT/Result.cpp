//
// Created by belyaev on 4/6/15.
//

#include "SMT/Result.h"

#include "Util/macros.h"

namespace borealis {
namespace smt {

SatResult::SatResult() :
    modelPtr(nullptr),
    initialMemoryShapePtr(nullptr),
    finalMemoryShapePtr(nullptr) {}

SatResult::SatResult(
    std::shared_ptr<model_t> modelPtr,
    std::shared_ptr<memory_shape_t> initialShapePtr,
    std::shared_ptr<memory_shape_t> finalShapePtr
) : modelPtr(modelPtr),
    initialMemoryShapePtr(initialShapePtr),
    finalMemoryShapePtr(finalShapePtr) {}

util::option_ref<Term::Ptr> SatResult::at(const std::string& str) const {
    return util::at(*modelPtr, str);
}

util::option_ref<Term::Ptr> SatResult::deref(uintptr_t ptr) const {
    return util::at(*finalMemoryShapePtr, ptr);
}

util::option<long long> SatResult::valueOf(const std::string& str) const {
    if (auto&& v = at(str)) {
        if (auto&& ii = llvm::dyn_cast<OpaqueIntConstantTerm>(v.getUnsafe())) {
            return util::just(ii->getValue());
        } else {
            UNREACHABLE("Non-integer value in model");
        }
    } else {
        return util::nothing();
    }
}

util::option<long long> SatResult::derefValueOf(uintptr_t ptr) const {
    if (auto&& v = deref(ptr)) {
        if (auto&& ii = llvm::dyn_cast<OpaqueIntConstantTerm>(v.getUnsafe())) {
            return util::just(ii->getValue());
        } else {
            UNREACHABLE("Non-integer value in memory");
        }
    } else {
        return util::nothing();
    }
}

std::ostream& operator<<(std::ostream& ost, const SatResult& res) {
    for (auto&& kv : *res.modelPtr) {
        ost << kv.first << " -> " << kv.second << std::endl;
    }
    return ost;
}

} // namespace smt
} // namespace borealis

#include "Util/unmacros.h"
