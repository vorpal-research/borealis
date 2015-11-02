/*
 * CallSiteInitializer.h
 *
 *  Created on: Nov 20, 2012
 *      Author: ice-phoenix
 */

#ifndef CALLSITEINITIALIZER_H_
#define CALLSITEINITIALIZER_H_

#include <llvm/IR/Argument.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <unordered_map>

#include "State/Transformer/Transformer.hpp"

#include "Util/macros.h"

namespace borealis {

class CallSiteInitializer : public borealis::Transformer<CallSiteInitializer> {

    using Base = borealis::Transformer<CallSiteInitializer>;

public:

    CallSiteInitializer(llvm::ImmutableCallSite CI, FactoryNest FN);

    Predicate::Ptr transformPredicate(Predicate::Ptr p);
    Annotation::Ptr transformRequiresAnnotation(RequiresAnnotationPtr p);
    Annotation::Ptr transformEnsuresAnnotation(EnsuresAnnotationPtr p);

    Term::Ptr transformArgumentTerm(ArgumentTermPtr t);
    Term::Ptr transformArgumentCountTerm(ArgumentCountTermPtr t);
    Term::Ptr transformVarArgumentTerm(VarArgumentTermPtr t);
    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr);
    Term::Ptr transformValueTerm(ValueTermPtr t);

private:

    using CallSiteArguments = std::unordered_map<unsigned int, const llvm::Value*>;

    llvm::ImmutableCallSite ci;
    std::string prefix;

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CALLSITEINITIALIZER_H_ */
