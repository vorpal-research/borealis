/*
 * AggregateFunctionPass.hpp
 *
 *  Created on: Oct 24, 2012
 *      Author: belyaev
 */

#ifndef AGGREGATEFUNCTIONPASS_HPP_
#define AGGREGATEFUNCTIONPASS_HPP_

#include <array>

#include <llvm/Pass.h>

#include "util.h"

namespace {
    using llvm::Function;
    using llvm::AnalysisUsage;

    using borealis::util::view;
    using borealis::util::get_index_of_T_in;
}



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
        return static_cast<T&>(*children[get_index_of_T_in<T, Passes...>::value]);
    }

    AggregateFunctionPass(char& ID):
        FunctionPass(ID) {
        children = {{ new Passes(this)... }};
    }

    virtual bool doInitialization(Module& M) {
        for(FunctionPass* child: children) {
            child->doInitialization(M);
        }
        return false;
    }

    virtual bool doFinalization(Module& M) {
        for(FunctionPass* child: view(children.rbegin(), children.rend())) {
            child->doFinalization(M);
        }
        return false;
    }


    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
        for(FunctionPass* child: children) {
            child->getAnalysisUsage(AU);
        }
    }
    virtual bool runOnFunction(Function& F) {
        for(FunctionPass* child: children) {
            child->runOnFunction(F);
        }
        return false;
    }


    virtual ~AggregateFunctionPass() {
        // TODO: make deletion type-aware?
        for(FunctionPass* child: children) {
            delete child;
        }
    }
};



#endif /* AGGREGATEFUNCTIONPASS_HPP_ */
