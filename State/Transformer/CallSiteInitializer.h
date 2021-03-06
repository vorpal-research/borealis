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
    CallSiteInitializer(llvm::ImmutableCallSite CI, FactoryNest FN, const Locus* loc);

    Predicate::Ptr transformPredicate(Predicate::Ptr p);
    Annotation::Ptr transformRequiresAnnotation(RequiresAnnotationPtr p);
    Annotation::Ptr transformEnsuresAnnotation(EnsuresAnnotationPtr p);
    Annotation::Ptr transformGlobalAnnotation(GlobalAnnotationPtr p);

    Term::Ptr transformArgumentTerm(ArgumentTermPtr t);
    Term::Ptr transformArgumentCountTerm(ArgumentCountTermPtr t);
    Term::Ptr transformVarArgumentTerm(VarArgumentTermPtr t);
    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr);
    Term::Ptr transformReturnPtrTerm(ReturnPtrTermPtr);
    Term::Ptr transformValueTerm(ValueTermPtr t);

private:

    using CallSiteArguments = std::unordered_map<unsigned int, const llvm::Value*>;

    llvm::ImmutableCallSite ci;
    std::string prefix;
    const Locus* overrideLoc;

    template<class ...Args>
    void failWith(const char* error, Args&&... args) {
        throw std::runtime_error {
            tfm::format(
                "CallSiteInitializer error while processing callsite %s: %s",
                llvm::valueSummary(ci.getInstruction()),
                tfm::format(error, std::forward<Args>(args)...)
            )
        };
    }

    template<class ...Args>
    void failWith(const std::string& error, Args&&... args) {
        return failWith(error.c_str(), std::forward<Args>(args)...);
    }

};

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* CALLSITEINITIALIZER_H_ */
