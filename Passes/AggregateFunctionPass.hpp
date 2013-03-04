/*
 * AggregateFunctionPass.hpp
 *
 *  Created on: Oct 24, 2012
 *      Author: belyaev
 */

#ifndef AGGREGATEFUNCTIONPASS_HPP_
#define AGGREGATEFUNCTIONPASS_HPP_

#include <llvm/Pass.h>

#include <array>

#include "Util/util.h"

namespace borealis {

// This is purely a base class.
// Will not work as an actual pass.
// Handling the usual pass routines is a clear derived classes' responsibility.
// Thanx.

template<class ...Passes>
class AggregateFunctionPass: public llvm::FunctionPass {

    std::array< FunctionPass*, (sizeof...(Passes)) > children;

public:

    template<class T>
    T& getChildAnalysis() {
        using borealis::util::get_index_of_T_in;
        return static_cast<T&>(*children[get_index_of_T_in<T, Passes...>::value]);
    }

    AggregateFunctionPass(char& ID):
        FunctionPass(ID) {
        children = {{ new Passes(this)... }};
    }

    virtual bool doInitialization(llvm::Module& M) {
        for (FunctionPass* child : children) {
            child->doInitialization(M);
        }
        return false;
    }

    virtual bool doFinalization(llvm::Module& M) {
        using borealis::util::reverse;
        for (FunctionPass* child : reverse(children)) {
            child->doFinalization(M);
        }
        return false;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        for (FunctionPass* child : children) {
            child->getAnalysisUsage(AU);
        }
    }

    virtual bool runOnFunction(llvm::Function& F) {
        for (FunctionPass* child : children) {
            child->runOnFunction(F);
        }
        return false;
    }

    virtual ~AggregateFunctionPass() {
        // TODO: make deletion type-aware?
        for (FunctionPass* child : children) {
            delete child;
        }
    }
};

} // namespace borealis

#endif /* AGGREGATEFUNCTIONPASS_HPP_ */
