/*
 * TermRenamer.h
 *
 *  Created on: Jul 11, 2013
 *      Author: ice-phoenix
 */

#ifndef TERMRENAMER_H_
#define TERMRENAMER_H_

#include <llvm/Instruction.h>
#include <llvm/Instructions.h>

#include "State/Transformer/Transformer.hpp"
#include "Util/util.h"

namespace borealis {

class TermRenamer : public borealis::Transformer<TermRenamer> {

public:

    TermRenamer(llvm::CallInst& CI) {
        using borealis::util::toString;

        auto* callerFunc = CI.getParent()->getParent();
        auto* callerInst = &CI;

        auto callerFuncName = callerFunc ? callerFunc->getName().str() : toString(callerFunc);
        auto callerInstName = callerInst ? callerInst->getName().str() : toString(callerInst);

        prefix = callerFuncName + "." + callerInstName + ".";
    }

    Term::Ptr transformValueTerm(ValueTermPtr t) {
        auto renamed = prefix + t->getName();
        return t->withNewName(renamed);
    }

private:

    std::string prefix;

};

} /* namespace borealis */

#endif /* TERMRENAMER_H_ */
