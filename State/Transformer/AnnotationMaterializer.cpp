/*
 * AnnotationMaterializer.cpp
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#include "State/Transformer/AnnotationMaterializer.h"
#include "Term/TermBuilder.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

struct AnnotationMaterializer::AnnotationMaterializerImpl {
    const LogicAnnotation* A;
    TermFactory::Ptr TF;
    VariableInfoTracker* MI;
    NameContext nc;
};

AnnotationMaterializer::AnnotationMaterializer(
        const LogicAnnotation& A,
        FactoryNest FN,
        VariableInfoTracker* MI) :
            Base(FN),
            pimpl(
                new AnnotationMaterializerImpl {
                    &A,
                    FN.Term,
                    MI,
                    NameContext{ NameContext::Placement::GlobalScope, nullptr, A.getLocus() }
                }
            ) {

    FN.Type->initialize(*MI);

    if (llvm::isa<EnsuresAnnotation>(A) ||
        llvm::isa<RequiresAnnotation>(A) ||
        llvm::isa<AssignsAnnotation>(A)) {
        pimpl->nc.placement = NameContext::Placement::OuterScope;
        if (auto* f = llvm::dyn_cast_or_null<llvm::Function>(
                pimpl->MI->locate(pimpl->nc.loc, DiscoveryPolicy::NextFunction).val
        )) {
            pimpl->nc.func = f;
        }
    } else if (llvm::isa<AssertAnnotation>(A) ||
               llvm::isa<AssumeAnnotation>(A)) {
        pimpl->nc.placement = NameContext::Placement::InnerScope;
        if (auto* f = llvm::dyn_cast_or_null<llvm::Function>(
                pimpl->MI->locate(pimpl->nc.loc, DiscoveryPolicy::PreviousFunction).val
        )) {
            pimpl->nc.func = f;
        }
    }
};

AnnotationMaterializer::AnnotationMaterializer(
        const LogicAnnotation& A,
        FactoryNest FN,
        VariableInfoTracker* MI,
        llvm::Function* F) :
            Base(FN),
            pimpl(
                new AnnotationMaterializerImpl {
                    &A,
                    FN.Term,
                    MI,
                    NameContext{ NameContext::Placement::GlobalScope, nullptr, A.getLocus() }
                }
            ) {

    FN.Type->initialize(*MI);

    if (llvm::isa<EnsuresAnnotation>(A) ||
        llvm::isa<RequiresAnnotation>(A) ||
        llvm::isa<AssignsAnnotation>(A)) {
        pimpl->nc.placement = NameContext::Placement::OuterScope;
        pimpl->nc.func = F;
    } else failWith("Cannot bind annotation to an external function");
};

AnnotationMaterializer::~AnnotationMaterializer() {}

Annotation::Ptr AnnotationMaterializer::doit() {
    auto&& trm = transform(pimpl->A->getTerm());
    return pimpl->A->clone(trm);
}

llvm::LLVMContext& AnnotationMaterializer::getLLVMContext() const {
    return pimpl->MI->getLLVMContext();
}

VariableInfoTracker::ValueDescriptor AnnotationMaterializer::forName(const std::string& name) const {
    switch (pimpl->nc.placement) {
    case NameContext::Placement::GlobalScope:
    case NameContext::Placement::InnerScope:
        return pimpl->MI->locate(name, pimpl->A->getLocus(), DiscoveryPolicy::PreviousVal);
    case NameContext::Placement::OuterScope:
        return pimpl->MI->locate(name, pimpl->A->getLocus(), DiscoveryPolicy::NextVal);
    }
}

const NameContext& AnnotationMaterializer::nameContext() const {
    return pimpl->nc;
}

TermFactory& AnnotationMaterializer::factory() const {
    return *pimpl->TF;
}

VariableInfoTracker::ValueDescriptors AnnotationMaterializer::forValue(llvm::Value* value) const {
    return pimpl->MI->locate(value);
}

VariableInfoTracker::ValueDescriptor AnnotationMaterializer::forValueSingle(llvm::Value* value) const {
    auto&& descs = forValue(value);
    ASSERTC(descs.size() == 1);
    return descs.front();
}

void AnnotationMaterializer::failWith(const std::string& message) {
    static std::string buf;

    std::ostringstream str;
    str << "Error while processing annotation: "
        << pimpl->A->toString()
        << "; scope "
        << pimpl->nc
        << ": "
        << message;

    buf = str.str();
    throw std::runtime_error(buf.c_str());
}

Term::Ptr AnnotationMaterializer::transformOpaqueCall(OpaqueCallTermPtr trm) {
    if (auto* builtin = llvm::dyn_cast<OpaqueBuiltinTerm>(trm->getLhv())) {
        if (builtin->getVName() == "property") {
            auto&& rhv = trm->getRhv().toVector();
            if (rhv.size() != 2) failWith("Illegal \\property access " + trm->getName() + ": exactly two operands expected");

            auto&& prop = FN.Term->getOpaqueConstantTerm(rhv[0]->getName());
            auto&& val = this->transform(rhv[1]);

            return FN.Term->getReadPropertyTerm(FN.Type->getInteger(32), prop, val);

        } else if (builtin->getVName() == "bound") {
            auto&& rhv = trm->getRhv().toVector();
            if (rhv.size() != 1) failWith("Illegal \\bound access " + trm->getName() + ": exactly one operand expected");

            auto&& val = this->transform(rhv[0]);

            return FN.Term->getBoundTerm(val);

        } else if (builtin->getVName() == "is_valid_ptr") {
            auto&& rhv = trm->getRhv().toVector();
            if (rhv.size() != 1) failWith("Illegal \\is_valid_ptr access " + trm->getName() + ": exactly one operand expected");

            auto&& val = this->transform(rhv[0]);
            auto&& type = val->getType();

            if (auto* ptrType = llvm::dyn_cast<type::Pointer>(type)) {
                auto&& pointed = ptrType->getPointed();

                auto&& bval = builder(val);

                return bval != null()
                       && bval != invalid()
                       && bval.bound().uge( builder(TypeUtils::getTypeSizeInElems(pointed)) );
            } else {
                failWith("Illegal \\is_valid_ptr access " + trm->getName() + ": called on non-pointer");
            }
        } else if (builtin->getVName() == "old") {
            // all \old's should be already taken care of, let's try to guess the problem
            if (trm->getRhv().size() != 1) failWith("Illegal \\old invocation " + trm->getName() + ": exactly one operand expected");
            else failWith("Malformed \\old invocation");
        } else {
            failWith("Cannot call " + trm->getName() + ": not supported");
        }
    } else {
        failWith("Cannot call " + trm->getName() + ": only builtins can be called in this way");
    }

    BYE_BYE(Term::Ptr, "Unreachable!");
}

Term::Ptr AnnotationMaterializer::transformDirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
    auto&& arg = trm->getLhv();

    auto&& load = llvm::dyn_cast<LoadTerm>(arg);
    if (not load)
        failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not an instance of load"
        );

    auto&& daStruct = llvm::dyn_cast<type::Record>(load->getType());
    if (not daStruct)
        failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not a structure type"
        );

    auto&& field = daStruct->getBody()->get().getFieldByName(trm->getProperty());
    if (not field)
        failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": no such member defined or structure not available on " +
            util::toString(*daStruct)
        );

    return *builder(load->getRhv()).gep(builder(0), builder(field.getUnsafe().getIndex()));
}

Term::Ptr AnnotationMaterializer::transformIndirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
    auto&& arg = trm->getLhv();

    auto&& daPointer = llvm::dyn_cast<type::Pointer>(arg->getType());
    if (not daPointer)
        failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not a pointer"
        );

    auto&& daStruct = llvm::dyn_cast<type::Record>(daPointer->getPointed());
    if (not daStruct)
        failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": term is not a pointer to a structure type"
        );

    auto&& field = daStruct->getBody()->get().getFieldByName(trm->getProperty());
    if (not field)
        failWith(
            "Cannot access member " +
            trm->getProperty() +
            ": no such member defined or structure not available on " +
            util::toString(*daStruct)
        );

    return *builder(arg).gep(builder(0), builder(field.getUnsafe().getIndex()));
}

Term::Ptr AnnotationMaterializer::transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
    return trm->isIndirect() ?
           transformIndirectOpaqueMemberAccessTerm(trm) :
           transformDirectOpaqueMemberAccessTerm(trm);
}

Term::Ptr AnnotationMaterializer::transformArrayOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
    auto&& arg = trm->getLhv();

    auto&& load = llvm::dyn_cast<LoadTerm>(arg);
    if (not load) failWith("Cannot access member: term is not an instance of load");

    auto&& daArray = llvm::dyn_cast<type::Array>(load->getType());
    if (not daArray) failWith("Cannot access element: term is not an array type");

    return *builder(load->getRhv()).gep(builder(0), trm->getRhv());
}

Term::Ptr AnnotationMaterializer::transformOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
    // FIXME: handle multidimensional arrays
    if (llvm::isa<type::Array>(trm->getType())) {
        return transformArrayOpaqueIndexingTerm(trm);
    }

    auto&& gep = builder(trm->getLhv()).gep(trm->getRhv())();

    // check types
    if (auto* terr = llvm::dyn_cast<type::TypeError>(gep->getType())) {
        failWith(terr->getMessage());
    } else if (llvm::isa<type::Pointer>(gep->getType())) {
        return *builder(gep);
    } else {
        failWith("Type " + util::toString(gep->getType()) + " not dereferenceable");
    }

    BYE_BYE(Term::Ptr, "Unreachable!");
}

Term::Ptr AnnotationMaterializer::transformOpaqueVarTerm(OpaqueVarTermPtr trm) {
    auto&& ret = forName(trm->getVName());
    if (ret.isInvalid()) failWith(trm->getVName() + " : variable not found in scope");

//    FIXME: akhin WTF???
//    auto&& bcType = ret.val->getType();
//    if (ret.shouldBeDereferenced) {
//        if (not bcType->isPointerTy()) failWith("wtf");
//        bcType = bcType->getPointerElementType();
//    }

    auto&& shouldBeDereferenced = ret.shouldBeDereferenced;
    // FIXME: Need to sort out memory model
    //        Global arrays and structures break things...
    if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(ret.val->getType())) {
        shouldBeDereferenced &= ptrType->getPointerElementType()->isSingleValueType();
    }

    auto&& var = factory().getValueTerm(ret.val, ret.type.getSignedness());

    if (shouldBeDereferenced) {
        return *builder(var);
    } else {
        return var;
    }
}

Term::Ptr AnnotationMaterializer::transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr trm) {
    const llvm::StringRef name{ trm->getVName() };
    auto&& ctx = nameContext();

    if (name == "result") {
        if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
            if(ctx.func->getReturnType()->isVoidTy()){
                failWith("\\result cannot be used with functions returning void");
            }

            auto&& desc = forValueSingle(ctx.func);
            auto&& funcType = DISubroutineType(desc.type);
            ASSERTC(funcType);
            auto&& diContext = pimpl->MI->getDebugInfo();
            auto&& retType_ = stripAliases(diContext, funcType.getReturnType());
            return factory().getReturnValueTerm(ctx.func, retType_.getSignedness());

        } else {
            failWith("\\result can only be bound to functions' outer scope");
        }

    } else if (name == "null" || name == "nullptr") {
        return null();

    } else if (name == "invalid" || name == "invalidptr") {
        return invalid();

    } else if (name.startswith("arg")) {
        if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
            std::istringstream ist(name.drop_front(3).str());
            unsigned val = 0U;
            ist >> val;

            if (ctx.func->isDeclaration()) failWith("annotations on function declarations are not supported");

            ASSERTC(val < ctx.func->arg_size());

            auto&& arg = std::next(ctx.func->arg_begin(), val);

            auto&& desc = forValueSingle(&*arg);
            return factory().getArgumentTerm(arg, desc.type.getSignedness());

        } else {
            failWith("\\argXXX can only be bound to functions' outer scope");
        }
    } else {
        failWith("\\" + name + " : unknown builtin");
    }

    BYE_BYE(Term::Ptr, "Unreachable!");
};

Term::Ptr AnnotationMaterializer::transformCmpTerm(CmpTermPtr trm) {
    auto&& lhvt = trm->getLhv()->getType();
    auto&& rhvt = trm->getRhv()->getType();

    // XXX: Tricky stuff follows...
    //      CmpTerm from annotations is signed by default,
    //      need to change that to unsigned when needed
    if (auto match = util::match_pair<type::Integer, type::Integer>(lhvt, rhvt)) {
        if (
            match->first->getSignedness() == llvm::Signedness::Unsigned ||
            match->second->getSignedness() == llvm::Signedness::Unsigned
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

Annotation::Ptr materialize(
        Annotation::Ptr annotation,
        FactoryNest FN,
        VariableInfoTracker* MI
) {
    if (auto* logic = llvm::dyn_cast<LogicAnnotation>(annotation)){
        AnnotationMaterializer am(*logic, FN, MI);
        return am.doit();
    } else {
        return annotation;
    }
}

} // namespace borealis

#include "Util/unmacros.h"
