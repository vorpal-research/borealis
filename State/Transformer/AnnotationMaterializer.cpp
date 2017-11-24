/*
 * AnnotationMaterializer.cpp
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#include "State/Transformer/AnnotationMaterializer.h"

#include "Codegen/CType/CTypeUtils.h"
#include "Codegen/CType/CTypeInfer.h"
#include "Term/TermBuilder.h"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {

struct AnnotationMaterializer::AnnotationMaterializerImpl {
    const LogicAnnotation* A;
    TermFactory::Ptr TF;
    VariableInfoTracker* MI;
    NameContext nc;
    CTypeInfer ctypeInfer;

    CType::Ptr propertyCType() {
        return MI->getCTypeFactory().getInteger("bor_property_t", 8, llvm::Signedness::Unknown);
    }

    CType::Ptr intConstantCType() {
        return ctypeInfer.getPropertyType();
    }

    CType::Ptr boolCType() {
        return ctypeInfer.getBoolType();
    }

    Term::Ptr withCType(Term::Ptr term, CType::Ptr type) {
        ctypeInfer.hintType(term, type);
        return term;
    }

    template<class ...Args>
    inline void failWith(const char* fmt, Args&&... args) {
        throw std::runtime_error(
            tfm::format(
                "Error while processing annotation: %s; scope %s: %s",
                A->toString(),
                nc,
                tfm::format(fmt, std::forward<Args>(args)...)
            )
        );
    }

    template<class ...Args>
    inline void abort(const char* fmt, Args&&... args) {
        throw aborted(
            tfm::format(
                "Error while processing annotation: %s; scope %s: %s",
                A->toString(),
                nc,
                tfm::format(fmt, std::forward<Args>(args)...)
            )
        );
    }

    template<class ...Args>
    inline void require(bool what, const char* fmt, Args&&... args) {
        if(!what) failWith(fmt, std::forward<Args>(args)...);
    }
};

static size_t getNextUniqueNumber() {
    static size_t current = 0;
    return current++;
}

static size_t nameToValueMapping(const std::string& key) {
    static std::unordered_map<std::string, size_t> data;
    auto end = std::end(data);
    auto it = data.find(key);
    if(it == end) {
        return data[key] = getNextUniqueNumber();
    } else {
        return it->second;
    }
}

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
                    NameContext{ NameContext::Placement::GlobalScope, nullptr, A.getLocus() },
                    CTypeInfer(MI->getCTypeFactory())
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
    } else if(llvm::isa<GlobalAnnotation>(A)){
        pimpl->nc.placement = NameContext::Placement::GlobalScope;
        pimpl->nc.func = nullptr;
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
                    NameContext{ NameContext::Placement::GlobalScope, F, A.getLocus() },
                    CTypeInfer(MI->getCTypeFactory())
                }
            ) {

    FN.Type->initialize(*MI);
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
        return pimpl->MI->locate(name, pimpl->A->getLocus(), DiscoveryPolicy::Global);
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
    ASSERTC(descs.size() >= 1);
    return descs.front();
}

Term::Ptr AnnotationMaterializer::transformOpaqueCall(OpaqueCallTermPtr trm) {
    if (auto* builtin = llvm::dyn_cast<OpaqueBuiltinTerm>(trm->getLhv())) {
        if (builtin->getVName() == "property") {
            auto&& rhv = trm->getRhv().toVector();
            pimpl->require(
                rhv.size() == 2,
                "Illegal \\property access %s: exactly two operands expected",
                trm->getName()
            );

            auto&& prop = FN.Term->getOpaqueConstantTerm(rhv[0]->getName());
            auto&& val = this->transform(rhv[1]);

            return FN.Term->getReadPropertyTerm(FN.Type->getInteger(), prop, val);
        } else if (builtin->getVName() == "bound") {
            auto&& rhv = trm->getRhv().toVector();
            pimpl->require(
                rhv.size() == 1,
                "Illegal \\bound access %s: exactly one operand expected",
                trm->getName()
            );

            auto&& val = this->transform(rhv[0]);

            return FN.Term->getBoundTerm(val);
        } else if (builtin->getVName() == "is_valid_ptr") {
            auto&& rhv = trm->getRhv().toVector();
            pimpl->require(
                rhv.size() == 1,
                "Illegal \\is_valid_ptr access %s: exactly one operand expected",
                trm->getName()
            );

            auto&& val = this->transform(rhv[0]);
            auto&& type = val->getType();

            if (auto* ptrType = llvm::dyn_cast<type::Pointer>(type)) {
                auto&& pointed = ptrType->getPointed();

                auto&& bval = builder(val);

                return bval != null()
                    && bval != invalid()
                    && bval.bound().uge( builder(TypeUtils::getTypeSizeInElems(pointed)) );
            } else {
                // every non-pointer is a valid pointer. Why? Because otherwise
                // functions with polymorphic arguments will not be handled correctly
                pimpl->abort("Illegal \\is_valid_ptr access %s: called on non-pointer", val->getName());
            }
        } else if (builtin->getVName() == "is_valid_array") {
            auto&& rhv = trm->getRhv().toVector();
            pimpl->require(
                rhv.size() == 2,
                "Illegal \\is_valid_array access %s: exactly two operands expected",
                trm->getName()
            );

            auto&& val = this->transform(rhv[0]);
            auto&& type = val->getType();

            auto&& size = this->transform(rhv[1]);

            if (auto* ptrType = llvm::dyn_cast<type::Pointer>(type)) {
                auto&& pointed = ptrType->getPointed();

                auto&& bval = builder(val);
                auto&& bound = bval.bound();

                return bval != null()
                    && bval != invalid()
                    && bound.uge(builder(TypeUtils::getTypeSizeInElems(pointed)) * builder(size).cast(bound->getType()));
            } else {
                pimpl->abort("Illegal \\is_valid_array access %s: called on non-pointer", val->getName());
            }
        } else if (builtin->getVName() == "umax") {
            auto&& rhv = trm->getRhv().map(APPLY(this->transform)).toVector();
            pimpl->require(
                rhv.size() == 2,
                "Illegal \\umax invocation %s: exactly two operands expected",
                trm->getName()
            );
            auto&& arg0 = builder(rhv[0]);
            auto&& arg1 = builder(rhv[1]);

            return FN.Term->getTernaryTerm(arg0.uge(arg1), arg0, arg1);

        } else if (builtin->getVName() == "umin") {
            auto&& rhv = trm->getRhv().map(APPLY(this->transform)).toVector();
            pimpl->require(
                rhv.size() == 2,
                "Illegal \\umin invocation %s: exactly two operands expected",
                trm->getName()
            );

            auto&& arg0 = builder(rhv[0]);
            auto&& arg1 = builder(rhv[1]);

            return FN.Term->getTernaryTerm(arg1.uge(arg0), arg0, arg1);
        } else if (builtin->getVName() == "old") {
            // all \old's should be already taken care of, let's try to guess the problem
            if (trm->getRhv().size() != 1)
                pimpl->failWith(
                    "Illegal \\old invocation %s: exactly one operand expected", trm->getName());
            else pimpl->failWith("Malformed \\old invocation");
        } else if (builtin->getVName() == "free"){
            pimpl->require(
                trm->getRhv().size() == 1,
                "Illegal \\free invocation %s: exactly one operand expected",
                trm->getName()
            );
            auto arg = trm->getRhv().first_or(nullptr);
            auto newName = tfm::format("$$free(%s)$$", arg->getName());
            return FN.Term->getFreeVarTerm(FN.Type->getInteger(), newName);
        } else {
            pimpl->failWith("Cannot call %s: not supported", trm->getName());
        }
    } else {
        pimpl->failWith("Cannot call %s: only builtins can be called in this way", trm->getName());
    }

    BYE_BYE(Term::Ptr, "Unreachable!");
}

Term::Ptr AnnotationMaterializer::transformDirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
    auto&& arg = trm->getLhv();

    auto&& load = llvm::dyn_cast<LoadTerm>(arg);
    pimpl->require(!!load, "Cannot access member %s: term is not an instance of load", trm->getProperty());

    auto&& daStruct = llvm::dyn_cast<type::Record>(load->getType());
    pimpl->require(!!daStruct, "Cannot access member %s: term is not a structure type", trm->getProperty());

    auto&& cdaStruct = pimpl->ctypeInfer.inferType(arg);
    auto&& cfield = CTypeUtils::getField(cdaStruct, trm->getProperty());
    pimpl->require(!!cfield, "Cannot access member %s: no such member defined or structrure not available for %s",
                   trm->getProperty(), cdaStruct->getName());

    auto&& structBody = daStruct->getBody()->get();
    auto index = structBody.offsetToIndex(cfield->getOffset());
    pimpl->require(index != ~size_t(0), "Cannot access member %s: no field by offset %d defined for %s",
                   trm->getProperty(), cfield->getOffset(), util::toString(*daStruct));

    auto&& base = builder(load->getRhv());
    auto&& gep = base.gep(builder(0), builder(index));

    return pimpl->withCType(*gep, cfield->getType());
}

Term::Ptr AnnotationMaterializer::transformIndirectOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
    auto&& arg = trm->getLhv();

    auto&& daPointer = llvm::dyn_cast<type::Pointer>(arg->getType());
    pimpl->require(!!daPointer, "Cannot access member %s: term is not a pointer", trm->getProperty());

    auto&& daStruct = llvm::dyn_cast<type::Record>(daPointer->getPointed());
    pimpl->require(!!daStruct, "Cannot access member %s: term is not a pointer to a structure type", trm->getProperty());

    auto&& cdaStructPtr = pimpl->ctypeInfer.inferType(arg);
    auto&& cdaStruct = CTypeUtils::loadType(cdaStructPtr);
    auto&& cfield = CTypeUtils::getField(cdaStruct, trm->getProperty());
    pimpl->require(!!cfield, "Cannot access member %s: no such member defined or structrure not available for %s",
                  trm->getProperty(), cdaStructPtr->getName());

    auto&& structBody = daStruct->getBody()->get();
    auto index = structBody.offsetToIndex(cfield->getOffset());
    pimpl->require(index != ~size_t(0), "Cannot access member %s: no field by offset %d defined for %s",
                   trm->getProperty(), cfield->getOffset(), util::toString(*daStruct));

    auto&& gep = builder(arg).gep(builder(0), builder(index));
    return pimpl->withCType(*gep, cfield->getType());
}

Term::Ptr AnnotationMaterializer::transformOpaqueMemberAccessTerm(OpaqueMemberAccessTermPtr trm) {
    return trm->isIndirect() ?
           transformIndirectOpaqueMemberAccessTerm(trm) :
           transformDirectOpaqueMemberAccessTerm(trm);
}

Term::Ptr AnnotationMaterializer::transformArrayOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
    auto&& arg = trm->getLhv();

    auto&& load = llvm::dyn_cast<LoadTerm>(arg);
    pimpl->require(!!load, "Cannot access member: term is not an instance of load");

    auto&& daArray = llvm::dyn_cast<type::Array>(load->getType());
    pimpl->require(!!daArray, "Cannot access member: term is not an array type");

    auto&& base = builder(load->getRhv());
    auto gep = base.gep(builder(0), trm->getRhv());
    return pimpl->withCType(*gep, pimpl->ctypeInfer.inferType(trm));

}

Term::Ptr AnnotationMaterializer::transformOpaqueIndexingTerm(OpaqueIndexingTermPtr trm) {
    // FIXME: handle multidimensional arrays
    if (llvm::isa<type::Array>(trm->getType())) {
        return transformArrayOpaqueIndexingTerm(trm);
    }

    auto&& base = builder(trm->getLhv());
    auto&& gep = base.gep(trm->getRhv());

    // check types
    if (auto* terr = llvm::dyn_cast<type::TypeError>(gep->getType())) {
        pimpl->failWith("Type error: %s", terr->getMessage());
    } else if (llvm::isa<type::Pointer>(gep->getType())) {
        return pimpl->withCType(
            *gep,
            pimpl->ctypeInfer.inferType(trm)
        );
    } else {
        pimpl->failWith("Type %s not dereferenceable", trm->getType());
    }

    UNREACHABLE("transformOpaqueIndexingTerm");
}

Term::Ptr AnnotationMaterializer::transformOpaqueVarTerm(OpaqueVarTermPtr trm) {
    auto&& ret = forName(trm->getVName());
    pimpl->require(not ret.isInvalid(),
                   "%s: variable not found in scope", trm->getVName());

    auto&& shouldBeDereferenced = ret.shouldBeDereferenced;
    // FIXME: Need to sort out memory model
    //        Global arrays and structures break things
    if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(ret.val->getType())) {
        shouldBeDereferenced &= ptrType->getPointerElementType()->isSingleValueType();
    }

    auto&& var = builder(factory().getValueTerm(ret.val, CTypeUtils::getSignedness(ret.type)));

    auto res = shouldBeDereferenced? *var : var;
    return pimpl->withCType(res, ret.type);
}

Term::Ptr AnnotationMaterializer::transformOpaqueNamedConstantTerm(OpaqueNamedConstantTermPtr trm) {
    auto value = nameToValueMapping(trm->getVName());
    return FN.Term->getOpaqueConstantTerm(value, FN.Type->defaultTypeSize);
}

Term::Ptr AnnotationMaterializer::transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr trm) {
    const llvm::StringRef name{ trm->getVName() };
    auto&& ctx = nameContext();

    if (name == "result") {
        if (ctx.func && (ctx.placement == NameContext::Placement::OuterScope
            || ctx.placement == NameContext::Placement::GlobalScope)) {
            pimpl->require(!ctx.func->getReturnType()->isVoidTy(),
                           "\\result cannot be used with functions returning void");

            auto&& descs = forValue(ctx.func);
            if(descs.empty()) {
                warns() << "Cannot materialize function " <<  ctx.func->getName() << ", trying to deduce type from llvm" << endl;
            }

            auto ctype = descs.empty()? pimpl->ctypeInfer.tryCastLLVMTypeToCType(ctx.func->getType()->getPointerElementType()) : descs.front().type;

            auto&& funcType = llvm::dyn_cast<CFunction>(ctype);
            ASSERTC(funcType);
            auto&& retType_ = funcType->getResultType().get();

            if(is_one_of<CStruct, CArray>(retType_)) {
                /// XXX: indirect return
                return pimpl->withCType(
                    factory().getLoadTerm(factory().getReturnPtrTerm(ctx.func)),
                    retType_
                );
            }

            return pimpl->withCType(
                factory().getReturnValueTerm(ctx.func, CTypeUtils::getSignedness(retType_)),
                retType_
            );
        } else {
            pimpl->failWith("\\result can only be bound to functions' outer scope");
        }

    } else if (name == "null" || name == "nullptr") {
        // we treat CType of null as int, because that way it will infer correctly
        return null();
    } else if (name == "invalid" || name == "invalidptr") {
        return invalid();
    } else if (name == "arg_count" || name == "num_args") {
        if(!ctx.func) {
            pimpl->failWith("\\arg_count can only be bound to functions' outer scope");
        }

        if(!ctx.func->isVarArg()) {
            warns() << "\\" << name << " is used on a non-vararg function";
            return factory().getOpaqueConstantTerm(ctx.func->arg_size(), TypeFactory::defaultTypeSize);
        } else {
            return factory().getArgumentCountTerm();
        }
    } else if (name.startswith("arg")) {
        if (ctx.func && ctx.placement == NameContext::Placement::OuterScope) {
            unsigned val;
            if(name.drop_front(3).getAsInteger(10, val)) val = 0;

            pimpl->require(!ctx.func->isDeclaration(),
                           "annotations on function declarations are not supported");

            if(ctx.func->isVarArg() && val >= ctx.func->arg_size()) {
                return pimpl->withCType(
                    factory().getVarArgumentTerm(val - ctx.func->arg_size()),
                    pimpl->ctypeInfer.getSmallIntegerType()
                );
            }

            if(!ctx.func->isVarArg() && val >= ctx.func->arg_size()) {
                pimpl->failWith("Function %s does not have argument %d", ctx.func->getName(), val);
            }

            auto&& arg = std::next(ctx.func->arg_begin(), val);

            auto&& desc = forValueSingle(&*arg);

            return pimpl->withCType(
                factory().getArgumentTerm(arg, CTypeUtils::getSignedness(desc.type)),
                desc.type
            );

        } else if (ctx.func && ctx.placement == NameContext::Placement::GlobalScope) { // external function
            unsigned val;
            if(name.drop_front(3).getAsInteger(10, val)) val = 0;

            if(ctx.func->isVarArg() && val >= ctx.func->arg_size()) {
                return pimpl->withCType(
                    factory().getVarArgumentTerm(val - ctx.func->arg_size()),
                    pimpl->ctypeInfer.getSmallIntegerType()
                );
            }

            if(!ctx.func->isVarArg() && val >= ctx.func->arg_size()) {
                pimpl->failWith("Function %s does not have argument %d", ctx.func->getName(), val);
            }

            auto&& descs = forValue(ctx.func);
            if(descs.empty()) {
                warns() << "Cannot materialize function " <<  ctx.func->getName() << ", trying to deduce type from llvm" << endl;
            }

            auto ctype = descs.empty()? pimpl->ctypeInfer.tryCastLLVMTypeToCType(ctx.func->getType()->getPointerElementType()) : descs.front().type;
            auto&& funcCType = llvm::dyn_cast<CFunction>(ctype);

            auto&& argCType = funcCType->getArgumentTypes()[val];
            auto&& argType = FN.Type->cast(ctx.func->getFunctionType()->getParamType(val), ctx.func->getDataLayout(), CTypeUtils::getSignedness(argCType));

            return pimpl->withCType(
                factory().getArgumentTermExternal(
                    val,
                    tfm::format("%s$arg%d", ctx.func->getName(), val),
                    argType
                ),
                argCType
            );

        } else {
            pimpl->failWith("\\argXXX can only be bound to functions' outer scope");
        }
    } else {
        pimpl->failWith("\\%s: unknown builtin", name);
    }

    BYE_BYE(Term::Ptr, "Unreachable!");
};

Term::Ptr AnnotationMaterializer::transformCmpTerm(CmpTermPtr trm) {
    auto&& lhvt = pimpl->ctypeInfer.inferType(trm->getLhv());
    auto&& rhvt = pimpl->ctypeInfer.inferType(trm->getRhv());
    auto&& merged = CTypeUtils::commonType(pimpl->MI->getCTypeFactory(), lhvt, rhvt);

    // XXX: Tricky stuff follows...
    //      CmpTerm from annotations is signed by default,
    //      need to change that to unsigned when needed
    if(CTypeUtils::getSignedness(merged) == llvm::Signedness::Unsigned) {
        return pimpl->withCType(
            factory().getCmpTerm(
                llvm::forceUnsigned(trm->getOpcode()),
                trm->getLhv(),
                trm->getRhv()
            ),
            pimpl->boolCType()
        );
    }

    return pimpl->withCType(trm, pimpl->boolCType());
}

Annotation::Ptr materialize(
        Annotation::Ptr annotation,
        FactoryNest FN,
        VariableInfoTracker* MI
) {
    Annotation::Ptr res;
    if (auto logic = llvm::dyn_cast<LogicAnnotation>(annotation)) {
        res = AnnotationMaterializer(*logic, FN, MI).doit();
    } else {
        res = annotation;
    }
    return res;
}


} // namespace borealis

#include "Util/unmacros.h"
