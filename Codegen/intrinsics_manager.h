/*
 * intrinsics_manager.h
 *
 *  Created on: Nov 15, 2012
 *      Author: ice-phoenix
 */

#ifndef INTRINSICS_MANAGER_H_
#define INTRINSICS_MANAGER_H_

#include <llvm/Function.h>

#include <unordered_map>

#include "intrinsics.h"

namespace borealis {

class IntrinsicsManager {

public:

    typedef std::unordered_map<llvm::Function*, intrinsic> intrinsics_type_cache;
    typedef std::unordered_map<llvm::Function*, PredicateState> intrinsics_state_cache;

    // TODO Make this shit also cache all intrinsic instances
    llvm::Function* createIntrinsic(
            intrinsic intr,
            llvm::Module* module,
            llvm::FunctionType* funtype,
            llvm::Type* param) {
        using llvm::GlobalValue;
        using llvm::Function;

        Function* f = Function::Create(
                funtype,
                GlobalValue::ExternalLinkage,
                getFuncName(intr, param),
                module
        );

        type_cache[f] = intr;

        return f;
    }

    bool isIntrinsic(llvm::Function& F) {
        return type_cache.count(&F) > 0;
    }

    intrinsic getIntrinsicType(llvm::Function& F) {
        auto iter = type_cache.find(&F);
        if (iter != type_cache.end()) {
            return (*iter).second;
        } else {
            return intrinsic::NOT_INTRINSIC;
        }
    }

    static IntrinsicsManager& getInstance() {
        static IntrinsicsManager instance;
        return instance;
    }

private:

    IntrinsicsManager() {};

    intrinsics_type_cache type_cache;
    intrinsics_state_cache state_cache;

};

} // namespace borealis

#endif /* INTRINSICS_MANAGER_H_ */
