/*
 * llvm_patterns.hpp
 *
 *  Created on: Jan 29, 2014
 *      Author: belyaev
 */

#ifndef LLVM_PATTERNS_HPP_
#define LLVM_PATTERNS_HPP_

#include <llvm/IR/PatternMatch.h>

#include "Util/util.h"

namespace llvm {
namespace PatternMatch {

template<class Ptr, class ...Idxs>
struct GetElementPointer_match {
    Ptr ptr;
    std::tuple<Idxs...> idxs;

    GetElementPointer_match(const Ptr& ptr, const Idxs&... idxs) : ptr{ptr}, idxs{idxs...} {};

    template<class OpTy>
    bool match(OpTy* opty) {
        return inner_match(opty, typename borealis::util::make_indexer<Idxs...>::type{});
    }

    template<class OpTy, size_t ...Ns>
    inline bool inner_match(OpTy* opty, borealis::util::indexer<Ns...>) {
        if(llvm::GetElementPtrInst* gep = llvm::dyn_cast<llvm::GetElementPtrInst>(opty)) {
            std::array<bool, sizeof...(Idxs)> biggies {
                { std::get<Ns>(idxs).match(gep->getOperand(Ns+1))... }
            };
            return ptr.match(gep->getPointerOperand())
                && std::all_of(std::begin(biggies), std::end(biggies), [](bool a){return a;});
        } else return false;
    }
};

template<class Ptr, class ...Idxs>
GetElementPointer_match<Ptr, Idxs...> m_GetElementPtrInst(const Ptr& ptr, const Idxs&... idxs) {
    return GetElementPointer_match<Ptr, Idxs...>{ ptr, idxs... };
}

template<class V, class ToType>
struct BitCast_with_type_match {
    V src;
    ToType totype;

    BitCast_with_type_match(const V& src, const ToType& totype) : src{ src }, totype{ totype } {};

    template<class OpTy>
    bool match(OpTy* opty) {
        if(llvm::BitCastInst* gep = llvm::dyn_cast<llvm::BitCastInst>(opty)) {
            return src.match(gep->getOperand(0))
                && totype.match(gep->getDestTy());
        } else return false;
    }
};

template<class V, class ToType>
BitCast_with_type_match<V, ToType> m_BitCast_toType(const V& ptr, const ToType& type) {
    return BitCast_with_type_match<V, ToType>{ ptr, type };
}

template<class Ty, class Delegate>
struct specifictype_ty {
    Delegate del;

    specifictype_ty(const Delegate& del): del{ del }{};

    template<typename IV>
    bool match(IV* V) {
        return llvm::TypeBuilder<Ty, true>::get(V->getContext()) == V->getType()
            && del.match(V);
    }
};

/// m_Specific - Match if we have a specific specified value.
template<class Ty, class Delegate>
specifictype_ty<Ty, Delegate> m_WithTypeTemplate(const Delegate& del) {
    return specifictype_ty<Ty, Delegate>{ del };
}

}
}

#endif /* LLVM_PATTERNS_HPP_ */
