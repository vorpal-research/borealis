/*
 * AnnotatorPass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#include "AnnotatorPass.h"

namespace borealis {

void AnnotatorPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const{
    using borealis::DataProvider;
    using borealis::comments::GatherCommentsAction;

    Info.addRequiredTransitive< comments >();
    Info.addRequiredTransitive< names >();
    Info.addRequiredTransitive< locs >();
}

bool AnnotatorPass::runOnModule(llvm::Module&) {
    using borealis::DataProvider;
    using borealis::comments::GatherCommentsAction;

    using llvm::errs;
    using borealis::util::streams::endl;
    using borealis::util::toString;
    using borealis::util::view;


    auto& commentsPass = getAnalysis< comments >();
    auto& locPass = getAnalysis< locs >();


    for(const auto & Comment: commentsPass.provide().getComments()) {
        const auto & loc = Comment.first;
        const auto & cmd = Comment.second;

        auto values = locPass.getRangeFor(loc);
        for(auto& it : view(values)) {
            annotations[it.second] = cmd;
        }
    }


    return false;
}

void AnnotatorPass::print(llvm::raw_ostream& O, const llvm::Module*) const {
    using borealis::util::streams::endl;
    using borealis::util::toString;

    for(const auto& An: annotations) {
        O << *An.first << endl;
        O << toString(An.second) << endl;
    }
}

char AnnotatorPass::ID;
}

namespace {
static llvm::RegisterPass<borealis::AnnotatorPass>
X("annotator", "Anno annotation language processor", false, false);
}



