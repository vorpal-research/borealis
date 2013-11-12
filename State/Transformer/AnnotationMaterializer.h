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
#include "Passes/Tracker/MetaInfoTracker.h"
#include "State/Transformer/Transformer.hpp"
#include "Term/NameContext.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

class AnnotationMaterializer : public borealis::Transformer<AnnotationMaterializer> {

    typedef borealis::Transformer<AnnotationMaterializer> Base;

    class AnnotationMaterializerImpl;
    std::unique_ptr<AnnotationMaterializerImpl> pimpl;

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

    MetaInfoTracker::ValueDescriptors forValue(llvm::Value* value) const;
    MetaInfoTracker::ValueDescriptor forValueSingle(llvm::Value* value) const {
        auto descs = forValue(value);
        ASSERTC(descs.size() == 1);
        return descs.front();
    }

    Annotation::Ptr doit();

    void failWith(const std::string& message);
    inline void failWith(llvm::Twine twine) {
        failWith(twine.str());
    }
    // resolving ambiguity
    inline void failWith(const char* r) {
        failWith(std::string(r));
    }

    // note this is called without a "Term" at the end, meaning
    // it is called before (and instead) transforming children
    Term::Ptr transformOpaqueCall(OpaqueCallTermPtr trm) {
        if(auto builtin = llvm::dyn_cast<OpaqueBuiltinTerm>(trm->getLhv())) {
            // FIXME: implement some calls
            if(builtin->getVName() == "property") {
                auto rhv = trm->getRhv();
                if(rhv.size() != 2) failWith("Illegal property access " + trm->getName() + ": exactly two operands expected");

                auto val = this->transform(rhv[0]);
                auto prop = llvm::dyn_cast<OpaqueVarTerm>(rhv[1]);
                if(!prop) failWith("Illegal property access " + trm->getName() + ": second operand must be a property name");

                auto transformed_prop = FN.Term->getOpaqueConstantTerm(prop->getVName());

                return FN.Term->getReadPropertyTerm(FN.Type->getUnknownType(), transformed_prop, val);
            }
            if(builtin->getVName() == "bound") {
                auto rhv = trm->getRhv();
                if(rhv.size() != 1) failWith("Illegal bound access " + trm->getName() + ": exactly one operand expected");

                auto val = this->transform(rhv[0]);

                return FN.Term->getBoundTerm(val);
            }
            if(builtin->getVName() == "is_valid_ptr") {
                auto rhv = trm->getRhv();
                if(rhv.size() != 1) failWith("Illegal is_valid_ptr access " + trm->getName() + ": exactly one operand expected");

                auto val = this->transform(rhv[0]);

                return FN.Term->getBinaryTerm(
                    llvm::ArithType::LAND,
                    FN.Term->getCmpTerm(
                        llvm::ConditionType::NEQ,
                        val,
                        FN.Term->getNullPtrTerm()
                    ),
                    FN.Term->getCmpTerm(
                        llvm::ConditionType::NEQ,
                        val,
                        FN.Term->getInvalidPtrTerm()
                    )
                );
            }
            failWith("Cannot call " + trm->getName() + ": not supported");
        } else failWith("Cannot call " + trm->getName() + ": only builtins can be called in this way");
        return trm;
    }

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

        auto var = factory().getValueTerm(ret.val, ret.signedness);

        if (ret.shouldBeDereferenced) {
            return factory().getLoadTerm(var);
        } else {
            return var;
        }
    }

    Term::Ptr transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr trm) {
        const llvm::StringRef name = trm->getName();
        const auto& ctx = nameContext();

        if (name == "result") {
            if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
                auto desc = forValueSingle(ctx.func);
                return factory().getReturnValueTerm(ctx.func, desc.signedness);
            } else {
                failWith("\result can only be bound to functions' outer scope");
            }
        } else if (name == "null" || name == "nullptr") {
            return factory().getNullPtrTerm();
        } else if (name == "invalid" || name == "invalidptr") {
            return factory().getInvalidPtrTerm();
        } else if (name.startswith("arg")) {
            if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
                std::istringstream ist(name.drop_front(3).str());
                unsigned val = 0U;
                ist >> val;

                auto argIt = ctx.func->arg_begin();
                std::advance(argIt, val);

                auto desc = forValueSingle(argIt);

                return factory().getArgumentTerm(argIt, desc.signedness);
            } else {
                failWith("\arg# can only be bound to functions' outer scope");
            }
        } else {
            failWith("\\" + name + " : unknown builtin");
        }

        return trm;
    }

    Term::Ptr transformCmpTerm(CmpTermPtr trm) {
        using borealis::util::match_pair;

        auto lhvt = trm->getLhv()->getType();
        auto rhvt = trm->getRhv()->getType();

        // XXX: Tricky stuff follows...
        //      CmpTerm from annotations is signed by default,
        //      need to change that to unsigned when needed
        if (auto match = match_pair<type::Integer, type::Integer>(lhvt, rhvt)) {
            if (
                    (
                        match->first->getSignedness() == llvm::Signedness::Unsigned &&
                        match->second->getSignedness() != llvm::Signedness::Signed
                    ) || (
                        match->second->getSignedness() == llvm::Signedness::Unsigned &&
                        match->first->getSignedness() != llvm::Signedness::Signed
                    )
            ) {
                return factory().getCmpTerm(
                    llvm::forceUnsigned(trm->getOpcode()),
                    trm->getLhv(),
                    trm->getRhv()
                );
            }
        }

        return trm;
    }
};

Annotation::Ptr materialize(Annotation::Ptr, FactoryNest FN, MetaInfoTracker*);

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* ANNOTATIONMATERIALIZER_H_ */
