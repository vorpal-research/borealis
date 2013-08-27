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

struct ShouldBeModularized {};

struct ShouldBeLazyModularized {};

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

namespace impl_ {
template<class P>
struct AUX {
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
}

////////////////////////////////////////////////////////////////////////////////

template<
    class P,
    impl_::PassType pType = std::is_base_of<ShouldBeLazyModularized, P>::value ? impl_::PassType::LAZY_MODULARIZED :
                            std::is_base_of<ShouldBeModularized, P>::value ? impl_::PassType::MODULARIZED :
                            impl_::PassType::OTHER
>
struct AUX;

template<class P>
struct AUX<P, impl_::PassType::OTHER> : public impl_::AUX< P > {};

template<class P>
struct AUX<P, impl_::PassType::MODULARIZED> : public impl_::AUX< PassModularizer<P> > {};

template<class P>
struct AUX<P, impl_::PassType::LAZY_MODULARIZED> : public impl_::AUX< LazyPassModularizer<P> > {};

////////////////////////////////////////////////////////////////////////////////
//
// Custom getAnalysis implementation
//
////////////////////////////////////////////////////////////////////////////////

template<
    class P, // Pass which we try to get
    impl_::PassType pType = std::is_base_of<ShouldBeLazyModularized, P>::value ? impl_::PassType::LAZY_MODULARIZED :
                            std::is_base_of<ShouldBeModularized, P>::value ? impl_::PassType::MODULARIZED :
                            std::is_base_of<llvm::FunctionPass, P>::value ? impl_::PassType::FUNCTION :
                            impl_::PassType::OTHER
>
struct GetAnalysis;

template<class P>
struct GetAnalysis<P, impl_::PassType::LAZY_MODULARIZED> {

    typedef LazyPassModularizer<P> PP;

    static P& doit(llvm::Pass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >(F).getResultsForFunction(&F);
    }
    static P& doit(borealis::ProxyFunctionPass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >(F).getResultsForFunction(&F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::MODULARIZED>{

    typedef PassModularizer<P> PP;

    static P& doit(llvm::Pass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >(F).getResultsForFunction(&F);
    }
    static P& doit(borealis::ProxyFunctionPass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< PP >(F).getResultsForFunction(&F);
    }
};

template<class P>
struct GetAnalysis<P, impl_::PassType::FUNCTION> {
    static P& doit(llvm::Pass* pass, llvm::Function& F) {
        ASSERTC(!F.isDeclaration());
        return pass->getAnalysis< P >(F);
    }
    static P& doit(borealis::ProxyFunctionPass* pass, llvm::Function& F) {
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

namespace impl_ {
template<class P>
struct RegisterPass {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly,
            bool isAnalysis) {

        static llvm::RegisterPass< P >
        MX(PassArg, Name, CFGOnly, isAnalysis);

    }
};
}

////////////////////////////////////////////////////////////////////////////////

template<
    class P,
    impl_::PassType pType = std::is_base_of<ShouldBeLazyModularized, P>::value ? impl_::PassType::LAZY_MODULARIZED :
                            std::is_base_of<ShouldBeModularized, P>::value ? impl_::PassType::MODULARIZED :
                            impl_::PassType::OTHER
>
struct RegisterPass;

template<class P>
struct RegisterPass<P, impl_::PassType::OTHER> : public impl_::RegisterPass< P > {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) :
    impl_::RegisterPass<P>(PassArg, Name, CFGOnly, isAnalysis) {};
};

template<class P>
struct RegisterPass<P, impl_::PassType::MODULARIZED> : public impl_::RegisterPass< PassModularizer<P> > {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) :
    impl_::RegisterPass< PassModularizer<P> >(PassArg, Name, CFGOnly, isAnalysis) {};
};

template<class P>
struct RegisterPass<P, impl_::PassType::LAZY_MODULARIZED> : public impl_::RegisterPass< LazyPassModularizer<P> > {
    RegisterPass(
            const char* PassArg,
            const char* Name,
            bool CFGOnly = false,
            bool isAnalysis = false) :
    impl_::RegisterPass< LazyPassModularizer<P> >(PassArg, Name, CFGOnly, isAnalysis) {};
};

} // namespace borealis

#include "Util/unmacros.h"

#endif /* PASSES_HPP_ */
