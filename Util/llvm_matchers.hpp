//
// Created by belyaev on 11/16/15.
//

#ifndef LLVM_MATCHERS_HPP
#define LLVM_MATCHERS_HPP

#include <functional-hell/matchers_aux.hpp>

#include <llvm/IR/Instructions.h>
#include <llvm/ADT/ArrayRef.h>

namespace llvm {

using namespace functional_hell::matchers;

template<class Wha>
static auto pattern_matcher(Wha pattern) {
    return [&pattern](llvm::Value* v) { return !!(pattern >> v); };
}

static auto $OfType = functional_hell::matchers::function_as_pattern([](auto&& v){ return make_storage(v->getType()); });

template <class Derived, class F>
static auto fwdAsDynCast(llvm::Value* v, F f) -> decltype(f(cast<Derived>(v))) {
    if(auto der = dyn_cast<Derived>(v)) return f(der);
    else return functional_hell::matchers::no_match;
}

struct ExtractValueExtractor {

    storage_t<Value*, ArrayRef<unsigned>> unapply(Value* v) const {
        if (auto&& tt = dyn_cast<ExtractValueInst>(v)) {
            return make_storage(tt->getAggregateOperand(), tt->getIndices());
        } else {
            return {};
        }
    }

};

static auto $ExtractValueInst = functional_hell::matchers::make_pattern(ExtractValueExtractor{});

struct InsertValueExtractor {

    storage_t<Value*, Value*, ArrayRef<unsigned>> unapply(Value* v) const {
        if (auto&& tt = dyn_cast<InsertValueInst>(v)) {
            return functional_hell::matchers::make_storage(tt->getAggregateOperand(), tt->getInsertedValueOperand(), tt->getIndices());
        } else {
            return {};
        }
    }

};

static auto $InsertValueInst = functional_hell::matchers::make_pattern(InsertValueExtractor{});

struct LoadExtractor {

    storage_t<Value*> unapply(Value* v) const {
        if(auto&& tt = dyn_cast<LoadInst>(v)) {
            return make_storage(tt->getPointerOperand());
        } else {
            return {};
        }
    }

};

static auto $LoadInst = functional_hell::matchers::make_pattern(LoadExtractor{});

struct StoreExtractor {

    storage_t<Value*, Value*> unapply(Value* v) const {
        if(auto&& tt = dyn_cast<StoreInst>(v)) {
            return make_storage(tt->getPointerOperand(), tt->getValueOperand());
        } else return {};
    }

};

static auto $StoreInst = functional_hell::matchers::make_pattern(StoreExtractor{});

struct CallExtractor {
    auto unapply(Value* v) const {
        return fwdAsDynCast<CallInst>(v, [](auto&& cc) { return make_storage(cc->getCalledValue(), cc->arg_operands()); }  );
    }

};

static auto $CallInst = functional_hell::matchers::make_pattern(CallExtractor{});

struct IntrinsicExtractor {
    storage_t<Intrinsic::ID> unapply(Value* v) const {
        if(auto&& f = dyn_cast<Function>(v)) {
            if(f->isIntrinsic()) {
                return make_storage((Intrinsic::ID)f->getIntrinsicID());
            } else return {};
        } else return {};
    }
};

static auto $Intrinsic = functional_hell::matchers::make_pattern(IntrinsicExtractor{});

} /* namespace llvm */

#endif //LLVM_MATCHERS_HPP
