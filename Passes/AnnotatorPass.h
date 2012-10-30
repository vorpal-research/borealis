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

#include "../Anno/anno.h"
#include "DataProvider.hpp"
#include "SourceLocationTracker.h"
#include "NameTracker.h"
#include "../comments.h"

#include "../Logging/logger.hpp"

#include <log4cpp/FileAppender.hh>
#include <log4cpp/PatternLayout.hh>

namespace borealis {

class AnnotatorPass: public llvm::ModulePass, public
    borealis::logging::ClassLevelLogging<AnnotatorPass> {
public:
    static char ID;
    static constexpr decltype("annotator") loggerDomain() { return "annotator"; }

    AnnotatorPass(): llvm::ModulePass(ID) {};

    typedef borealis::DataProvider<borealis::comments::GatherCommentsAction> comments;
    typedef borealis::SourceLocationTracker locs;
    typedef borealis::NameTracker names;
    typedef unordered_map<Value*, borealis::anno::command> annotations_t;

private:
    annotations_t annotations;
public:

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const;
    virtual bool runOnModule(llvm::Module&);
    virtual void print(llvm::raw_ostream& O, const llvm::Module*) const;

};

}

#endif // ANNOTATOR_H
