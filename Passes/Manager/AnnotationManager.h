/*
 * AnnotationManager.h
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#ifndef ANNOTATIONMANAGER_H
#define ANNOTATIONMANAGER_H

#include <llvm/Pass.h>

#include <vector>

#include "Actions/comments.h"
#include "Anno/anno.h"
#include "Annotation/Annotation.h"
#include "Logging/logger.hpp"
#include "Passes/Tracker/SlotTrackerPass.h"
#include "Passes/Util/DataProvider.hpp"

namespace borealis {

class AnnotationManager:
    public llvm::ModulePass,
    public borealis::logging::ClassLevelLogging<AnnotationManager> {

public:
    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("annotator")
#include "Util/unmacros.h"

    AnnotationManager() : llvm::ModulePass(ID) {};
    virtual ~AnnotationManager() {};

    typedef DataProvider<borealis::comments::GatherCommentsAction> comments;
    typedef SlotTrackerPass slots;
    typedef std::vector< Annotation::Ptr > annotation_container;

private:
    annotation_container annotations;

public:

#include "Util/macros.h"
    auto begin() QUICK_RETURN(annotations.begin())
    auto end() QUICK_RETURN(annotations.end())
    auto rbegin() QUICK_RETURN(annotations.rbegin())
    auto rend() QUICK_RETURN(annotations.rend())
#include "Util/unmacros.h"

    const annotation_container& getAnnotations() { return annotations; }

    virtual void getAnalysisUsage(llvm::AnalysisUsage& Info) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream&, const llvm::Module*) const override;
};

} // namespace borealis

#endif // ANNOTATIONMANAGER_H
