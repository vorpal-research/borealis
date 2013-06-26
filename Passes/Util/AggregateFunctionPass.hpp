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

// This is a purely base class.
// Will not work as an actual pass. Ever.
// Handling the usual pass routines is a clear derived classes' responsibility.
// 10x.

template<class ...Passes>
class AggregateFunctionPass : public llvm::FunctionPass {

    std::array< FunctionPass*, sizeof...(Passes) > children;

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
        bool changed = false;
        for (auto* child : children) {
            changed |= child->doInitialization(M);
        }
        return changed;
    }

    virtual bool doFinalization(llvm::Module& M) {
        using borealis::util::reverse;

        bool changed = false;
        for (auto* child : reverse(children)) {
            changed |= child->doFinalization(M);
        }
        return changed;
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
        for (auto* child : children) {
            child->getAnalysisUsage(AU);
        }
    }

    virtual bool runOnFunction(llvm::Function& F) {
        bool changed = false;
        for (auto* child : children) {
            changed |= child->runOnFunction(F);
        }
        return changed;
    }

    virtual ~AggregateFunctionPass() {
        for (auto* child : children) {
            delete child;
        }
    }
};

} // namespace borealis

#endif /* AGGREGATEFUNCTIONPASS_HPP_ */
