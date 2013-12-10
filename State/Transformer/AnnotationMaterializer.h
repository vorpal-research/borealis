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
#include "Term/TermBuilder.h"
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
    // it is called before (and instead of) transforming children
    Term::Ptr transformOpaqueCall(OpaqueCallTermPtr trm) {
        if(auto builtin = llvm::dyn_cast<OpaqueBuiltinTerm>(trm->getLhv())) {
            if(builtin->getVName() == "property") {
                auto rhv = trm->getRhv();
                if(rhv.size() != 2) failWith("Illegal property access " + trm->getName() + ": exactly two operands expected");

                auto prop = FN.Term->getOpaqueConstantTerm(rhv[0]->getName());
                auto val = this->transform(rhv[1]);

                return FN.Term->getReadPropertyTerm(FN.Type->getUnknownType(), prop, val);

            } else if(builtin->getVName() == "bound") {
                auto rhv = trm->getRhv();
                if(rhv.size() != 1) failWith("Illegal bound access " + trm->getName() + ": exactly one operand expected");

                auto val = this->transform(rhv[0]);

                return FN.Term->getBoundTerm(val);

            } else if(builtin->getVName() == "is_valid_ptr") {
                auto rhv = trm->getRhv();
                if(rhv.size() != 1) failWith("Illegal is_valid_ptr access " + trm->getName() + ": exactly one operand expected");

                auto val = this->transform(rhv[0]);
                auto type = val->getType();

                if (auto ptrType = llvm::dyn_cast<type::Pointer>(type)) {
                    auto pointed = ptrType->getPointed();

                    auto bval = builder(val);

                    return bval != null()
                        && bval != invalid()
                        && bval.bound().uge (builder(TypeUtils::getTypeSizeInElems(pointed)));
                  } else failWith("Illegal is_valid_ptr access " + trm->getName() + ": called on non-pointer");

            } else failWith("Cannot call " + trm->getName() + ": not supported");

        } else failWith("Cannot call " + trm->getName() + ": only builtins can be called in this way");

        BYE_BYE(Term::Ptr, "Unreachable!");
    }

    Term::Ptr transformDirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
        auto arg = trm->getLhv();

        auto load = llvm::dyn_cast<LoadTerm>(arg);
        if(!load) failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not an instance of load"
        );

        auto daStruct = llvm::dyn_cast<type::Record>(load->getType());
        if(!daStruct) failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not a structure type"
        );

        auto field = daStruct->getBody()->get().getFieldByName(trm->getProperty());
        if(!field) failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": no such member defined or structure not available"
        );


        return *builder(load->getRhv()).gep(builder(0), builder(field.getUnsafe().getIndex()));

    }

    Term::Ptr transformIndirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
        auto arg = trm->getLhv();

        auto daPointer = llvm::dyn_cast<type::Pointer>(arg->getType());
        if(!daPointer) failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not a pointer"
        );

        auto daStruct = llvm::dyn_cast<type::Record>(daPointer->getPointed());
        if(!daStruct) failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not a pointer to a structure type"
        );

        auto field = daStruct->getBody()->get().getFieldByName(trm->getProperty());
        if(!field) failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": no such member defined or structure not available"
        );

        return *builder(arg).gep(builder(0), builder(field.getUnsafe().getIndex()));
    }

    Term::Ptr transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
        return trm->isIndirect() ?
               transformIndirectOpaqueMemberAccessTerm(trm) :
               transformDirectOpaqueMemberAccessTerm(trm);
    }

    Term::Ptr transformArrayOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
        auto arg = trm->getLhv();

        auto load = llvm::dyn_cast<LoadTerm>(arg);
        if(!load) failWith("Cannot access member: term is not an instance of load");

        auto daArray = llvm::dyn_cast<type::Array>(load->getType());
        if(!daArray) failWith("Cannot access element: term is not an array type");

        return *builder(load->getRhv()).gep(builder(0), trm->getRhv());

    }

    Term::Ptr transformOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
        // handle multidimensional arrays
        if(llvm::isa<type::Array>(trm->getType())) {
            return transformArrayOpaqueIndexingTerm(trm);
        }

        auto gep = builder(trm->getLhv()).gep(trm->getRhv())();

        // check types
        if(auto* terr = llvm::dyn_cast<type::TypeError>(gep->getType())) {
            failWith(terr->getMessage());
        } else if(llvm::isa<type::Pointer>(gep->getType())) {
            return factory().getLoadTerm(gep);
        } else failWith("Type " + util::toString(gep->getType()) + " not dereferenceable");

        BYE_BYE(Term::Ptr, "Unreachable!");
    }

    Term::Ptr transformOpaqueVarTerm(OpaqueVarTermPtr trm) {
        auto ret = forName(trm->getVName());
        if (ret.isInvalid()) failWith(trm->getVName() + " : variable not found in scope");

        auto bcType = ret.val->getType();
        if (ret.shouldBeDereferenced) {
            if(!bcType->isPointerTy()) failWith("wtf");
            bcType = bcType->getPointerElementType();
        }
        FN.Type->cast(bcType, ret.type); // side-effecting to load type metadata

        auto shouldBeDereferenced = ret.shouldBeDereferenced;
        // FIXME: Need to sort out memory model
        //        Global arrays and structures break things...
        if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(ret.val->getType())) {
            shouldBeDereferenced &= ptrType->getPointerElementType()->isSingleValueType();
        }

        auto var = factory().getValueTerm(ret.val, ret.type.getSignedness());

        if (shouldBeDereferenced) {
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
                return factory().getReturnValueTerm(ctx.func, desc.type.getSignedness());

            } else failWith("\result can only be bound to functions' outer scope");

        } else if (name == "null" || name == "nullptr") {
            return factory().getNullPtrTerm();

        } else if (name == "invalid" || name == "invalidptr") {
            return factory().getInvalidPtrTerm();

        } else if (name.startswith("arg")) {
            if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
                std::istringstream ist(name.drop_front(3).str());
                unsigned val = 0U;
                ist >> val;

                ASSERTC(val < ctx.func->arg_size());

                auto arg = ctx.func->arg_begin();
                std::advance(arg, val);

                auto desc = forValueSingle(arg);

                return factory().getArgumentTerm(arg, desc.type.getSignedness());
            } else {
                failWith("\argXXX can only be bound to functions' outer scope");
            }

        } else {
            failWith("\\" + name + " : unknown builtin");
        }

        BYE_BYE(Term::Ptr, "Unreachable!");
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
