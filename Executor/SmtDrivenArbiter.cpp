//
// Created by belyaev on 4/7/15.
//

#include "Executor/SmtDrivenArbiter.h"

#include "Term/TermUtils.hpp"

#include "Util/macros.h"

static llvm::GenericValue integerRepToGeneric(const std::string& rep, llvm::Type* tp) {
    llvm::GenericValue ret;
    ret.IntVal = llvm::APInt(tp->getIntegerBitWidth(), rep, 10);
    return ret;
}

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

    auto term = FN.Term->getValueTerm(val);
    auto qres = model.getModel().query(term);

    if(auto&& v = TermUtils::getBoolValue(qres)) {
        return adjustIntegerToType(v.getUnsafe(), val->getType());
    } else if(auto&& v = TermUtils::getIntegerValue(qres)) {
        return adjustIntegerToType(v.getUnsafe(), val->getType());
    } else if(auto&& v = TermUtils::getIntegerValueRep(qres)) {
        return integerRepToGeneric(v.getUnsafe(), val->getType());
    }else {
        return adjustIntegerToType(-0xdeadbeef, val->getType());
    }
}

#include "Util/unmacros.h"
