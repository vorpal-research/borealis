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
#include "DataProvider.hpp"
#include "Logging/logger.hpp"
#include "NameTracker.h"
#include "SourceLocationTracker.h"

namespace borealis {

class AnnotatorPass:
    public llvm::ModulePass,
    public borealis::logging::ClassLevelLogging<AnnotatorPass> {

public:

    static char ID;
    static constexpr decltype("annotator") loggerDomain() { return "annotator"; }

    AnnotatorPass(): llvm::ModulePass(ID) {};

    typedef DataProvider<borealis::comments::GatherCommentsAction> comments;
    typedef SourceLocationTracker locs;
    typedef NameTracker names;
    typedef std::unordered_map<llvm::Value*, borealis::anno::command> annotations_t;

private:

    annotations_t annotations;

public:

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);
    virtual void print(llvm::raw_ostream& O, const llvm::Module*) const;
};

} // namespace borealis

#endif // ANNOTATOR_H
