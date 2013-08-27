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

#include "Passes/Util/PassModularizer.hpp"
#include "Passes/Util/ProxyFunctionPass.h"

#include "Util/macros.h"

namespace borealis {

class ShouldBeModularized {};

class ShouldBeLazyModularized {};

namespace impl_ {
enum class PassType {
    LAZY_MODULARIZED,
    MODULARIZED,
    FUNCTION,
    OTHER
};
}

////////////////////////////////////////////////////////////////////////////////
//
// Custom AnalysisUsage implementation
//
////////////////////////////////////////////////////////////////////////////////

template<
    class P,
    impl_::PassType =
        std::is_base_of<ShouldBeLazyModularized, P>::value ? impl_::PassType::LAZY_MODULARIZED :
        std::is_base_of<ShouldBeModularized, P>::value     ? impl_::PassType::MODULARIZED :
        impl_::PassType::OTHER
>
struct AUX;

template<class P>
struct AUX<P, impl_::PassType::OTHER> {
    static void addRequiredTransitive(llvm::AnalysisUsage& AU) {
        AU.addRequiredTransitive< P >();
    }
    static void addRequired(llvm::AnalysisUsage& AU) {
        AU.addRequired< P >();
    }
    static void addPreserved(llvm::AnalysisUsage& AU) {
        AU.addPreserved< P >();
    }
};

template<class P>
struct AUX<P, impl_::PassType::MODULARIZED> {

    typedef PassModularizer<P> PP;

    static void addRequiredTransitive(llvm::AnalysisUsage& AU) {
        AU.addRequiredTransitive< PP >();
    }
    static void addRequired(llvm::AnalysisUsage& AU) {
        AU.addRequired< PP >();
    }
    static void addPreserved(llvm::AnalysisUsage& AU) {
        AU.addPreserved< PP >();
    }
};


template<class P>
struct AUX<P, impl_::PassType::LAZY_MODULARIZED> {

    typedef LazyPassModularizer<P> PP;

    static void addRequiredTransitive(llvm::AnalysisUsage& AU) {
        AU.addRequiredTransitive< PP >();
    }
    static void addRequired(llvm::AnalysisUsage& AU) {
        AU.addRequired< PP >();
    }
    static void addPreserved(llvm::AnalysisUsage& AU) {
        AU.addPreserved< PP >();
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// Custom getAnalysis implementation
//
////////////////////////////////////////////////////////////////////////////////

template<
    class P,
    impl_::PassType pType =
        std::is_base_of<ShouldBeLazyModularized, P>::value ? impl_::PassType::LAZY_MODULARIZED :
        std::is_base_of<ShouldBeModularized, P>::value     ? impl_::PassType::MODULARIZED :
        std::is_base_of<llvm::FunctionPass, P>::value      ? impl_::PassType::FUNCTION :
        impl_::PassType::OTHER
>
struct GetAnalysis;

template<class P>
struct GetAnalysis<P, impl_::PassType::LAZY_MODULARIZED> {

    typedef LazyPassModularizer<P> PP;

    static P& doit(const llvm::Pass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >().getResultsForFunction(&F);
    }
    static P& doit(const borealis::ProxyFunctionPass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >().getResultsForFunction(&F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::MODULARIZED> {

    typedef PassModularizer<P> PP;

    static P& doit(const llvm::Pass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >().getResultsForFunction(&F);
    }
    static P& doit(const borealis::ProxyFunctionPass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >().getResultsForFunction(&F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::FUNCTION> {
    static P& doit(const llvm::Pass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< P >(F);
    }
    static P& doit(const borealis::ProxyFunctionPass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< P >(F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::OTHER> {
    static P& doit(const llvm::Pass* pass) {
        return pass->getAnalysis< P >();
    }
    static P& doit(const llvm::Pass* pass, llvm::Function&) {
        return pass->getAnalysis< P >();
    }
    static P& doit(const borealis::ProxyFunctionPass* pass) {
        return pass->getAnalysis< P >();
    }
    static P& doit(const borealis::ProxyFunctionPass* pass, llvm::Function&) {
        return pass->getAnalysis< P >();
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// Custom RegisterPass implementation
//
////////////////////////////////////////////////////////////////////////////////

template<
    class P,
    impl_::PassType =
        std::is_base_of<ShouldBeLazyModularized, P>::value ? impl_::PassType::LAZY_MODULARIZED :
        std::is_base_of<ShouldBeModularized, P>::value     ? impl_::PassType::MODULARIZED :
        impl_::PassType::OTHER
>
struct RegisterPass;

template<class P>
struct RegisterPass<P, impl_::PassType::OTHER> {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) {

        static llvm::RegisterPass< P >
        MX(PassArg, Name, CFGOnly, isAnalysis);

    }
};

template<class P>
struct RegisterPass<P, impl_::PassType::MODULARIZED> {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) {

        static llvm::RegisterPass< PassModularizer<P> >
        MX(PassArg, Name, CFGOnly, isAnalysis);

    }
};

template<class P>
struct RegisterPass<P, impl_::PassType::LAZY_MODULARIZED> {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) {

        static llvm::RegisterPass< LazyPassModularizer<P> >
        MX(PassArg, Name, CFGOnly, isAnalysis);

    }
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PASSES_HPP_ */
