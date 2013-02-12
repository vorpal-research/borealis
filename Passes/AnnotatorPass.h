/*
 * Annotator.h
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#ifndef ANNOTATOR_H
#define ANNOTATOR_H

#include <unordered_map>

#include <llvm/Function.h>
#include <llvm/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include "Actions/comments.h"
#include "Anno/anno.h"
#include "Annotation/Annotation.h"
#include "Logging/logger.hpp"
#include "Passes/DataProvider.hpp"

namespace borealis {

class AnnotatorPass:
    public llvm::ModulePass,
    public borealis::logging::ClassLevelLogging<AnnotatorPass> {

public:
    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("annotator")
#include "Util/unmacros.h"

    AnnotatorPass(): llvm::ModulePass(ID) {};

    typedef DataProvider<borealis::comments::GatherCommentsAction> comments;
    typedef std::vector< Annotation::Ptr > annotation_container;

private:
    annotation_container annotations;

public:

#include "Util/macros.h"
    auto begin() QUICK_RETURN(annotations.begin())
    auto end() QUICK_RETURN(annotations.end())
#include "Util/unmacros.h"

    const annotation_container& getAnnotations() { return annotations; }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);
    virtual void print(llvm::raw_ostream& O, const llvm::Module*) const;
};

} // namespace borealis

#endif // ANNOTATOR_H
