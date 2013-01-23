/*
 * AnnotatorPass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#include "AnnotatorPass.h"

#include "Term/Term.h"
#include "Term/TermFactory.h"

#include "Annotation/AnnotationCast.h"

namespace borealis {


void AnnotatorPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const{
    Info.setPreservesAll();
    Info.addRequiredTransitive< comments >();
}

bool AnnotatorPass::runOnModule(llvm::Module&) {
    using borealis::util::view;

    auto& commentsPass = getAnalysis< comments >();

    auto tf = TermFactory::get(nullptr);

    for (const auto & Comment : commentsPass.provide().getComments()) {
        const auto & loc = Comment.first;
        const auto & cmd = Comment.second;

        annotations.push_back(fromParseResult(loc, cmd, tf.get()));
    }

    return false;
}

void AnnotatorPass::print(llvm::raw_ostream& O, const llvm::Module*) const {
    using borealis::util::streams::endl;
    using borealis::util::toString;

    for(const auto& An : annotations) {
        O << *An << endl;
    }
}

char AnnotatorPass::ID;
static llvm::RegisterPass<borealis::AnnotatorPass>
X("annotator", "Anno annotation language processor", false, false);

} // namespace borealis
