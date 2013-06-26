/*
 * ProxyFunctionPass.h
 *
 *  Created on: Feb 11, 2013
 *      Author: ice-phoenix
 */

#ifndef PROXYFUNCTIONPASS_H_
#define PROXYFUNCTIONPASS_H_

#include <llvm/Pass.h>

// This is a purely base class.
// Will not work as an actual pass.
// Handling the usual pass routines is a clear derived classes' responsibility.
// 10x.

namespace borealis {

class ProxyFunctionPass : public llvm::FunctionPass {
    Pass* delegate;

public:

    ProxyFunctionPass(char& ID, Pass* delegate = nullptr);

    // here we overload (not override, but shadow-overload) all methods of llvm::Pass

#define DELEGATE(WHAT) \
    if (delegate) { \
        return delegate->WHAT; \
    } else { \
        return Pass::WHAT; \
    }

    template<typename AnalysisType>
    inline AnalysisType* getAnalysisIfAvailable() const {
        DELEGATE(getAnalysisIfAvailable<AnalysisType>())
    }

    inline bool mustPreserveAnalysisID(char &AID) const {
        DELEGATE(mustPreserveAnalysisID(AID))
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysis() const {
        DELEGATE(getAnalysis<AnalysisType>())
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysis(llvm::Function &F) {
        DELEGATE(getAnalysis<AnalysisType>(F))
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysisID(llvm::AnalysisID PI) const {
        DELEGATE(getAnalysisID<AnalysisType>(PI))
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysisID(llvm::AnalysisID PI, llvm::Function &F) {
        DELEGATE(getAnalysisID<AnalysisType>(PI, F));
    }

#undef DELEGATE

    virtual ~ProxyFunctionPass();
};

} // namespace borealis

#endif /* PROXYFUNCTIONPASS_H_ */
