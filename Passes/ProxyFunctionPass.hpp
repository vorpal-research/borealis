/*
 * ProxyFunctionPass.hpp
 *
 *  Created on: Oct 24, 2012
 *      Author: belyaev
 */

#ifndef PROXYFUNCTIONPASS_HPP_
#define PROXYFUNCTIONPASS_HPP_

#include <llvm/Pass.h>

// This is purely a base class.
// Will not work as an actual pass.
// Handling the usual pass routines is a clear derived classes' responsibility.
// Thanx.

namespace borealis {

template<class Master>
class ProxyFunctionPass : public llvm::FunctionPass {
    Pass* delegate;

public:

    ProxyFunctionPass(Pass* delegate = nullptr):
        FunctionPass(Master::ID), delegate(delegate) {};

    // here we overload (not override, but shadow-overload) all methods of Pass

    /// getAnalysisIfAvailable<AnalysisType>() - Subclasses use this function to
    /// get analysis information that might be around, for example to update it.
    /// This is different than getAnalysis in that it can fail (if the analysis
    /// results haven't been computed), so should only be used if you can handle
    /// the case when the analysis is not available.  This method is often used by
    /// transformation APIs to update analysis results for a pass automatically as
    /// the transform is performed.
    ///
    template<typename AnalysisType>
    inline AnalysisType* getAnalysisIfAvailable() const {
        if(delegate) {
            return delegate->getAnalysisIfAvailable<AnalysisType>();
        } else {
            return Pass::getAnalysisIfAvailable<AnalysisType>();
        }
    }

    /// mustPreserveAnalysisID - This method serves the same function as
    /// getAnalysisIfAvailable, but works if you just have an AnalysisID.  This
    /// obviously cannot give you a properly typed instance of the class if you
    /// don't have the class name available (use getAnalysisIfAvailable if you
    /// do), but it can tell you if you need to preserve the pass at least.
    ///
    inline bool mustPreserveAnalysisID(char &AID) const {
        if(delegate) {
            return delegate->mustPreserveAnalysisID(AID);
        } else {
            return Pass::mustPreserveAnalysisID(AID);
        }
    }

    /// getAnalysis<AnalysisType>() - This function is used by subclasses to get
    /// to the analysis information that they claim to use by overriding the
    /// getAnalysisUsage function.
    ///
    template<typename AnalysisType>
    inline AnalysisType &getAnalysis() const {
        if(delegate) {
            return delegate->getAnalysis<AnalysisType>();
        } else {
            return Pass::getAnalysis<AnalysisType>();
        }
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysis(llvm::Function &F) {
        if(delegate) {
            return delegate->getAnalysis<AnalysisType>(F);
        } else {
            return Pass::getAnalysis<AnalysisType>(F);
        }
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysisID(llvm::AnalysisID PI) const {
        if(delegate) {
            return delegate->getAnalysisID<AnalysisType>(PI);
        } else {
            return Pass::getAnalysisID<AnalysisType>(PI);
        }
    }

    template<typename AnalysisType>
    inline AnalysisType &getAnalysisID(llvm::AnalysisID PI, llvm::Function &F) {
        if(delegate) {
            return delegate->getAnalysisID<AnalysisType>(PI, F);
        } else {
            return Pass::getAnalysisID<AnalysisType>(PI, F);
        }
    }

    virtual ~ProxyFunctionPass() {};
};

} // namespace borealis

#endif /* PROXYFUNCTIONPASS_HPP_ */
