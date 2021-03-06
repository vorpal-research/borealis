//
// Created by belyaev on 11/16/15.
//

#ifndef LLVM_MATCHERS_HPP
#define LLVM_MATCHERS_HPP

#include <functional-hell/matchers_aux.hpp>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/ADT/ArrayRef.h>

#include "Util/macros.h"

namespace llvm {

using namespace functional_hell::matchers;

template<class Wha>
static auto pattern_matcher(Wha pattern) {
    return [&pattern](llvm::Value* v) { return static_cast<bool>(pattern >> v); };
}

static auto $OfType = function_as_pattern([](auto&& v){ return make_storage(v->getType()); });

template <class Derived, class V, class F>
inline auto fwdAsDynCast(V&& v, F f) -> decltype(f(cast<Derived>(v))) {
    if(auto der = dyn_cast<Derived>(std::forward<V>(v))) return f(der);
    else return no_match;
}

struct ExtractValueExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<ExtractValueInst>(v, LAM(tt, make_storage(tt->getAggregateOperand(), tt->getIndices())));
    }
};

static auto $ExtractValueInst = make_pattern(ExtractValueExtractor{});

struct InsertValueExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<InsertValueInst>(
            v,
            LAM(tt, make_storage(tt->getAggregateOperand(), tt->getInsertedValueOperand(), tt->getIndices()))
        );
    }
};

static auto $InsertValueInst = make_pattern(InsertValueExtractor{});

struct LoadExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<LoadInst>(
            v,
            LAM(tt, make_storage(tt->getPointerOperand()))
        );
    }
};

static auto $LoadInst = make_pattern(LoadExtractor{});

struct StoreExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<StoreInst>(
            v,
            LAM(tt, make_storage(tt->getPointerOperand(), tt->getValueOperand()))
        );
    }
};

static auto $StoreInst = make_pattern(StoreExtractor{});

struct CallExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<CallInst>(v, LAM(cc, make_storage(cc->getCalledValue(), cc->arg_operands())));
    }
};

static auto $CallInst = make_pattern(CallExtractor{});

struct IntrinsicExtractor {
    storage_t<Intrinsic::ID> unapply(Value* v) const {
        return fwdAsDynCast<Function>(
            v,
            LAM(f, f->isIntrinsic()? make_storage(Intrinsic::ID(f->getIntrinsicID())) : no_match)
        );
    }
};

static auto $Intrinsic = make_pattern(IntrinsicExtractor{});

struct ICmpExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<ICmpInst>(
                v,
                LAM(tt, make_storage(tt->getOperand(0), tt->getOperand(1), tt->getPredicate()))
        );
    }
};

static auto $ICmpInst = make_pattern(ICmpExtractor{});

struct FCmpExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<FCmpInst>(
                v,
                LAM(tt, make_storage(tt->getOperand(0), tt->getOperand(1), tt->getPredicate()))
        );
    }
};

static auto $FCmpInst = make_pattern(FCmpExtractor{});

struct BinaryExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<BinaryOperator>(
                v,
                LAM(tt, make_storage(tt->getOperand(0), tt->getOperand(1), tt->getOpcode()))
        );
    }
};

static auto $BinaryOperator = make_pattern(BinaryExtractor{});

} /* namespace llvm */

#include "Util/unmacros.h"

#endif //LLVM_MATCHERS_HPP
