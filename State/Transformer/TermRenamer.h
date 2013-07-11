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

namespace borealis {

class TermRenamer : public borealis::Transformer<TermRenamer> {

public:

    TermRenamer(llvm::CallInst& CI) {
        auto callerFuncName = CI.getParent()->getParent()->getName().str();
        auto callerInstName = CI.getName().str();
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
