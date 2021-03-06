/*
 * AnnotationMaterializer.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONMATERIALIZER_H_
#define ANNOTATIONMATERIALIZER_H_

#include <memory>
#include <sstream>

#include "Annotation/Annotation.def"
#include "Passes/Tracker/VariableInfoTracker.h"
#include "State/Transformer/Transformer.hpp"
#include "Term/NameContext.h"

namespace borealis {

class AnnotationMaterializer : public borealis::Transformer<AnnotationMaterializer> {

    using Base = borealis::Transformer<AnnotationMaterializer>;

    struct AnnotationMaterializerImpl;
    std::unique_ptr<AnnotationMaterializerImpl> pimpl;

public:

    AnnotationMaterializer(
            const LogicAnnotation& A,
            FactoryNest FN,
            VariableInfoTracker* MI);


    AnnotationMaterializer(
            const LogicAnnotation& A,
            FactoryNest FN,
            VariableInfoTracker* MI,
            llvm::Function* externalFunction);
    ~AnnotationMaterializer();

    llvm::LLVMContext& getLLVMContext() const;
    VariableInfoTracker::ValueDescriptor forName(const std::string& name) const;
    const NameContext& nameContext() const;
    TermFactory& factory() const;

    VariableInfoTracker::ValueDescriptors forValue(llvm::Value* value) const;
    VariableInfoTracker::ValueDescriptor forValueSingle(llvm::Value* value) const;

    Annotation::Ptr doit();

    // note this is called without a "Term" at the end, meaning
    // it is called before (and instead of) transforming children
    Term::Ptr transformOpaqueCall(OpaqueCallTermPtr trm);

    Term::Ptr transformDirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm);
    Term::Ptr transformIndirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm);
    Term::Ptr transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm);
    Term::Ptr transformArrayOpaqueIndexingTerm(OpaqueIndexingTermPtr trm);
    Term::Ptr transformOpaqueIndexingTerm(OpaqueIndexingTermPtr trm);
    Term::Ptr transformOpaqueVarTerm(OpaqueVarTermPtr trm);
    Term::Ptr transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr trm);
    Term::Ptr transformOpaqueNamedConstantTerm(OpaqueNamedConstantTermPtr trm);

    Term::Ptr transformCmpTerm(CmpTermPtr trm);

    struct aborted: std::runtime_error{ using std::runtime_error::runtime_error; };
};

Annotation::Ptr materialize(Annotation::Ptr, FactoryNest FN, VariableInfoTracker*);

} /* namespace borealis */

#endif /* ANNOTATIONMATERIALIZER_H_ */
