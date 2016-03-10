#ifndef EXECUTOR_ANNOTATIONEXECUTOR_H_
#define EXECUTOR_ANNOTATIONEXECUTOR_H_

#include <memory>

#include <llvm/IR/Instructions.h>


#include "Passes/Tracker/SlotTrackerPass.h"
#include "State/Transformer/Transformer.hpp"


namespace borealis {

class ExecutionEngine;

class AnnotationExecutor: public Transformer<AnnotationExecutor> {
    struct Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    AnnotationExecutor(FactoryNest FN, SlotTracker* ST, ExecutionEngine* ee);
    ~AnnotationExecutor();

    Annotation::Ptr transformAssertAnnotation(AssertAnnotationPtr a);
    Annotation::Ptr transformAssumeAnnotation(AssumeAnnotationPtr a);
    Annotation::Ptr transformRequiresAnnotation(RequiresAnnotationPtr a);
    Annotation::Ptr transformEnsuresAnnotation(EnsuresAnnotationPtr a);

    Term::Ptr transformValueTerm(ValueTermPtr t);
    Term::Ptr transformUnaryTerm(UnaryTermPtr t);
    Term::Ptr transformBinaryTerm(BinaryTermPtr t);
    Term::Ptr transformTernaryTerm(TernaryTermPtr t);
    Term::Ptr transformSignTerm(SignTermPtr t);
    Term::Ptr transformReturnValueTerm(ReturnValueTermPtr t);
    Term::Ptr transformReturnPtrTerm(ReturnPtrTermPtr t);
    Term::Ptr transformArgumentTerm(ArgumentTermPtr t);
    Term::Ptr transformGepTerm(GepTermPtr t);
    Term::Ptr transformLoadTerm(LoadTermPtr t);
    Term::Ptr transformCmpTerm(CmpTermPtr t);
    Term::Ptr transformAxiomTerm(AxiomTermPtr t);

    Term::Ptr transformOpaqueUndefTerm(OpaqueUndefTermPtr t);

    Term::Ptr transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr t);
    Term::Ptr transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr t);
    Term::Ptr transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr t);
    Term::Ptr transformOpaqueBigIntConstantTerm(OpaqueBigIntConstantTermPtr t);
    Term::Ptr transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr t);
    Term::Ptr transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr t);
};

inline void executeAnnotation(Annotation::Ptr annotation, FactoryNest FN, ExecutionEngine* ee) {
    AnnotationExecutor(FN, FN.Slot, ee).transform(annotation);
}

} /* namespace borealis */

#endif /* EXECUTOR_ANNOTATIONEXECUTOR_H_ */
