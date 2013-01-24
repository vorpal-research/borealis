/*
 * cast.hpp
 *
 *  Created on: Jan 24, 2013
 *      Author: belyaev
 */

#ifndef CAST_HPP_
#define CAST_HPP_

#include <llvm/Support/Casting.h>
#include "Util/meta.hpp"
#include "Util/option.hpp"

namespace borealis{
namespace util{

template<class Basic>
class llvm_rtti_visitor {
    const Basic* b;

public:
    llvm_rtti_visitor(const Basic* b): b(b){};
    llvm_rtti_visitor(const Basic& b): b(&b){};
    llvm_rtti_visitor(const llvm_rtti_visitor& b) = default;

    template<class Derived, class Callable>
    auto on(Callable c) -> option<decltype(c(some<Derived>()))> {
        if(auto der = llvm::dyn_cast_or_null<Derived>(b)) {
            return just(c(*der));
        }
        return nothing();
    }

    template<class Derived, class Callable>
    llvm_rtti_visitor& oncase(Callable c) {
        if(auto der = llvm::dyn_cast_or_null<Derived>(b)) {
            c(*der);
        }
        return *this;
    }
};

template<class Basic>
llvm_rtti_visitor<Basic> visit(const Basic& b) {
    return b;
}

template<class Basic>
llvm_rtti_visitor<Basic> visit(const Basic* b) {
    return b;
}

}
}



#endif /* CAST_HPP_ */
