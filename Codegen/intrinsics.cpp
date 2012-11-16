/*
 * intrinsics.cpp
 *
 *  Created on: Oct 18, 2012
 *      Author: belyaev
 */

#include <unordered_map>

#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/Instructions.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Type.h>

#include "intrinsics.h"

using namespace borealis;

const std::string borealis::getFuncName(intrinsic intr, llvm::Type* type) {
    static std::unordered_map<intrinsic, std::string> names {
        { intrinsic::PTR_VERSION, "ptrver" },
    };

    std::string buf;
    llvm::raw_string_ostream oss(buf);

    oss << "borealis." << names[intr] << "." << *type;
    return oss.str();
}

PredicateState borealis::getPredicateState(
        intrinsic intr,
        llvm::Function* F,
        PredicateFactory* pf) {
    using namespace llvm;

    switch (intr) {

    case intrinsic::PTR_VERSION:
        {
            // `p = prtver(q)` => p == q
            auto* p = getReturnValue(F);
            auto* q = &*F->getArgumentList().begin();

            PredicateState res;
            res.addPredicate(pf->getEqualityPredicate(p, q));
            return res;
        }

    default:
        {
            return util::sayonara<PredicateState>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                    "Unknown intrinsic type");
        }
    }
}

llvm::Value* borealis::getReturnValue(llvm::Function* F) {
    using namespace llvm;

    for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
        if (isa<ReturnInst>(*I)) {
            ReturnInst& ret = cast<ReturnInst>(*I);
            return ret.getReturnValue();
        }
    }

    return nullptr;
}
