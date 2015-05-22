//
// Created by belyaev on 4/7/15.
//

#include "Executor/SmtDrivenArbiter.h"

#include "Util/macros.h"

static llvm::GenericValue adjustIntegerToType(long long i, llvm::Type* tp) {
    llvm::GenericValue ret;

    if(tp->isIntegerTy()) {
        ret.IntVal = llvm::APInt(tp->getIntegerBitWidth(), i);
    } else if(tp -> isPointerTy()) {
        ret.PointerVal = reinterpret_cast<void*>(static_cast<intptr_t>(i));
    } else if(tp -> isDoubleTy()) {
        ret.DoubleVal = i;
    } else if(tp -> isFloatTy()) {
        ret.FloatVal = i;
    } else UNREACHABLE("Type currently unsupported");

    return ret;
}

llvm::GenericValue borealis::SmtDrivenArbiter::map(llvm::Value* val) {

    auto name = ST->getLocalName(val);

    if(auto&& v = model.valueOf(name)) {
        auto numeric = v.getUnsafe();

        return adjustIntegerToType(numeric, val->getType());
    } else {
        return adjustIntegerToType(-0xdeadbeef, val->getType());
    }
}

#include "Util/unmacros.h"
