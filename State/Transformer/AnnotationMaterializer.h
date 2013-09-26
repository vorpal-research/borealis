/*
 * AnnotationMaterializer.h
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#ifndef ANNOTATIONMATERIALIZER_H_
#define ANNOTATIONMATERIALIZER_H_

#include <sstream>

#include "Annotation/Annotation.def"
#include "Passes/Tracker/MetaInfoTracker.h"
#include "State/Transformer/Transformer.hpp"
#include "Term/NameContext.h"
#include "Util/util.h"

namespace borealis {

class AnnotationMaterializer : public borealis::Transformer<AnnotationMaterializer> {

    typedef borealis::Transformer<AnnotationMaterializer> Base;

    class AnnotationMaterializerImpl;
    AnnotationMaterializerImpl* pimpl;

public:

    AnnotationMaterializer(
            const LogicAnnotation& A,
            FactoryNest FN,
            MetaInfoTracker* MI);
    ~AnnotationMaterializer();

    llvm::LLVMContext& getLLVMContext() const;
    MetaInfoTracker::ValueDescriptor forName(const std::string& name) const;
    const NameContext& nameContext() const;
    TermFactory& factory() const;

    Annotation::Ptr doit();

    void failWith(const std::string& message);
    inline void failWith(llvm::Twine twine) {
        failWith(twine.str());
    }
    // resolving ambiguity
    inline void failWith(const char* r) {
        failWith(std::string(r));
    }


    // note this is using the more general visitor form (not taking the args into account)
    Term::Ptr transformOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
        // FIXME: decide and handle the multidimensional array case

        auto gep = factory().getNaiveGepTerm(trm->getLhv(), util::make_vector(trm->getRhv()));

        // check typing
        if(const auto* terr = llvm::dyn_cast<type::TypeError>(gep->getType())){
            failWith(terr->getMessage());
        } else if(llvm::isa<type::Pointer>(gep->getType())) {
            return factory().getLoadTerm(gep);
        } else failWith("Type not dereferenceable");

        return trm;
    }

    Term::Ptr transformOpaqueVarTerm(OpaqueVarTermPtr trm) {
        auto ret = forName(trm->getVName());
        if (ret.isInvalid()) failWith(trm->getVName() + " : variable not found in scope");

        if (ret.shouldBeDereferenced) {
            return factory().getLoadTerm(factory().getValueTerm(ret.val));
        } else {
            return factory().getValueTerm(ret.val);
        }
    }

    Term::Ptr transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr trm) {
        const llvm::StringRef name = trm->getName();
        const auto& ctx = nameContext();

        if (name == "result") {
            if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
                return factory().getReturnValueTerm(ctx.func);
            } else {
                failWith("\result can only be bound to functions' outer scope");
            }
        } else if (name == "null" || name == "nullptr") {
            return factory().getNullPtrTerm();
        } else if (name.startswith("arg")) {
            if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
                std::istringstream ist(name.drop_front(3).str());
                unsigned val = 0U;
                ist >> val;

                auto argIt = ctx.func->arg_begin();
                std::advance(argIt, val);

                return factory().getArgumentTerm(argIt);
            } else {
                failWith("\arg# can only be bound to functions' outer scope");
            }
        } else {
            failWith("\\" + name + " : unknown builtin");
        }

        return trm;
    }
};

Annotation::Ptr materialize(Annotation::Ptr, FactoryNest FN, MetaInfoTracker*);

} /* namespace borealis */

#endif /* ANNOTATIONMATERIALIZER_H_ */
