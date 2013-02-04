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

#include "Codegen/intrinsics.h"

#include "Util/macros.h"

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
        PredicateFactory* PF,
        TermFactory* TF) {
    using namespace llvm;

    switch (intr) {

    case intrinsic::PTR_VERSION:
        {
            // `p = prtver(q)` => p == q
            PredicateState res;
            return res.addPredicate(
                PF->getEqualityPredicate(
                    TF->getReturnValueTerm(F),
                    TF->getArgumentTerm(&*F->getArgumentList().begin())
                )
            );
        }

    default:
        {
            BYE_BYE(PredicateState, "Unknown intrinsic type");
        }
    }
}

#include "Util/unmacros.h"
