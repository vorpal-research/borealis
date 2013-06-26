/*
 * AnnotatorPass.cpp
 *
 *  Created on: Oct 8, 2012
 *      Author: belyaev
 */

#include "Annotation/AnnotationCast.h"
#include "Passes/AnnotatorPass.h"
#include "Term/TermFactory.h"
#include "Util/passes.hpp"
#include "Util/util.h"

namespace borealis {

void AnnotatorPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const {
    AU.setPreservesAll();

    AUX< comments >::addRequiredTransitive(AU);
    AUX< slots >::addRequiredTransitive(AU);
}

bool AnnotatorPass::runOnModule(llvm::Module& M) {
    using borealis::util::view;

    auto& cmnts = GetAnalysis< comments >::doit(this);

    auto* st = GetAnalysis< slots >::doit(this).getSlotTracker(M);
    auto tf = TermFactory::get(st);

    for (const auto& c : cmnts.provide().getComments()) {
        const auto& loc = c.first;
        const auto& cmd = c.second;

        annotations.push_back(fromParseResult(loc, cmd, tf.get()));
    }

    return false;
}

void AnnotatorPass::print(llvm::raw_ostream&, const llvm::Module*) const {
    for (const auto& An : annotations) {
        infos() << An << endl;
    }
}

char AnnotatorPass::ID;
static RegisterPass<borealis::AnnotatorPass>
X("annotator", "Anno annotation language processor", false, false);

} // namespace borealis
