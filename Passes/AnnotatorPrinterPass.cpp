/*
 * AnnotatorPrinterPass.cpp
 *
 *  Created on: Jan 10, 2013
 *      Author: belyaev
 */

#include "AnnotatorPrinterPass.h"
#include "AnnotatorPass.h"

namespace borealis {

AnnotatorPrinterPass::AnnotatorPrinterPass(): ModulePass(ID) {}

AnnotatorPrinterPass::~AnnotatorPrinterPass() {}

char AnnotatorPrinterPass::ID;
static llvm::RegisterPass<AnnotatorPrinterPass>
X("anno-printer", "Anno annotation printer", false, false);

void AnnotatorPrinterPass::getAnalysisUsage(llvm::AnalysisUsage& Info) const {
    Info.setPreservesAll();
    Info.addRequired<AnnotatorPass>();
}

bool AnnotatorPrinterPass::runOnModule(llvm::Module&) {
    auto& anno = getAnalysis<AnnotatorPass>();

    infos() << anno;

    return false;
}

} /* namespace borealis */
