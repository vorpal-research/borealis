/*
 * passes.hpp
 *
 *  Created on: Feb 11, 2013
 *      Author: ice-phoenix
 */

#ifndef PASSES_HPP_
#define PASSES_HPP_

#include <llvm/Pass.h>

#include <type_traits>

#include "Passes/PassModularizer.hpp"

namespace borealis {

class ShouldBeModularized {};

template<class P, bool isModularized = std::is_base_of<ShouldBeModularized, P>::value>
struct AUX;

template<class P>
struct AUX<P, false> {
    static void addRequiredTransitive(llvm::AnalysisUsage& AU) {
        AU.addRequiredTransitive< P >();
    }
};

template<class P>
struct AUX<P, true> {
    static void addRequiredTransitive(llvm::AnalysisUsage& AU) {
        AU.addRequiredTransitive< PassModularizer<P> >();
    }
};

namespace impl_ {
enum class PassType {
    MODULARIZED,
    FUNCTION,
    OTHER
};
}

template<
    class P, // Pass which we try to get
    impl_::PassType pType = std::is_base_of<ShouldBeModularized, P>::value ? impl_::PassType::MODULARIZED :
                            std::is_base_of<llvm::FunctionPass, P>::value ? impl_::PassType::FUNCTION :
                            impl_::PassType::OTHER
>
struct GetAnalysis;

template<class P>
struct GetAnalysis<P, impl_::PassType::MODULARIZED> {
    static P& doit(llvm::Pass* pass, llvm::Function& F) {
        return pass->getAnalysis< PassModularizer<P> >().getResultsForFunction(&F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::FUNCTION> {
    static P& doit(llvm::Pass* pass, llvm::Function& F) {
        return pass->getAnalysis< P >(F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::OTHER> {
    static P& doit(llvm::Pass* pass) {
        return pass->getAnalysis< P >();
    }

    static P& doit(llvm::Pass* pass, llvm::Function& F) {
        return pass->getAnalysis< P >();
    }
};

} // namespace borealis

#endif /* PASSES_HPP_ */
