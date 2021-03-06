/*
 * AnnotationProcessor.h
 *
 *  Created on: Feb 14, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONPROCESSOR_H_
#define ANNOTATIONPROCESSOR_H_

#include <llvm/Pass.h>

namespace borealis {

class AnnotationProcessor : public llvm::ModulePass {

public:

    static char ID;

#include "Util/macros.h"
    static constexpr auto loggerDomain() QUICK_RETURN("annotator")
#include "Util/unmacros.h"

    AnnotationProcessor() : llvm::ModulePass(ID) {};
    virtual ~AnnotationProcessor() {};

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    virtual bool runOnModule(llvm::Module&) override;
    virtual void print(llvm::raw_ostream& O, const llvm::Module* M) const override;

    static bool landOnInstructionOrFirst(Annotation::Ptr anno,
                                        llvm::Module& M,
                                        FactoryNest FN,
                                        llvm::Value& val);
    static bool landOnInstructionOrLast(Annotation::Ptr anno,
                                        llvm::Module& M,
                                        FactoryNest FN,
                                        llvm::Value& val);
};

} /* namespace borealis */

#endif /* ANNOTATIONPROCESSOR_H_ */
