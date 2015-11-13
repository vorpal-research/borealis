#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>

#include "Executor/AnnotationExecutor.h"
#include "Executor/ExecutionEngine.h"
#include "Executor/Exceptions.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

static llvm::Type* mergeTypes(llvm::Type* lhv, llvm::Type* rhv, const llvm::DataLayout* dl) {
    TRACE_FUNC;

    if(lhv == rhv) return lhv;
    if(lhv->isVoidTy()) return rhv;
    if(rhv->isVoidTy()) return lhv;

    if(lhv->isIntegerTy() && rhv->isIntegerTy()) {
        size_t sz = std::max(lhv->getIntegerBitWidth(), rhv->getIntegerBitWidth());
        return llvm::Type::getIntNTy(lhv->getContext(), sz);
    }
    if(lhv->isFloatingPointTy() && rhv->isFloatingPointTy()) {
        bool left = dl->getTypeAllocSize(lhv) > dl->getTypeAllocSize(rhv);
        return left? lhv : rhv;
    }

    throw std::logic_error(tfm::format("unmergeable types: %s and %s", util::toString(*lhv), util::toString(*rhv)));
}

static void adjustToType(llvm::GenericValue& v, llvm::Type* pastType, llvm::Type* type) {
    TRACE_FUNC;

    if(pastType == type) return;
    if(type->isIntegerTy()) {
        auto sz = type->getIntegerBitWidth();
        if(v.IntVal.getBitWidth() != sz)
            v.IntVal = v.IntVal.sextOrTrunc(sz);
    } else if(type->isDoubleTy() && pastType->isFloatTy()) {
        v.DoubleVal = v.FloatVal;
    }   else if(type->isFloatTy() && pastType->isDoubleTy()) {
        v.FloatVal = v.DoubleVal;
    }
}

struct AnnotationExecutor::Impl {
    SlotTracker* ST;
    ExecutionEngine* ee;

    std::stack<std::pair<llvm::GenericValue, llvm::Type*>> value_stack;

    Impl(SlotTracker* ST, ExecutionEngine* ee):
        ST{ST}, ee{ee}, value_stack{} {};

    llvm::LLVMContext& getLLVMContext() const {
        return ee->getCurrentContext().CurFunction->getContext();
    }

    llvm::GenericValue pop() {
        ASSERTC(!value_stack.empty());
        llvm::GenericValue v = std::move(value_stack.top().first);
        value_stack.pop();
        return std::move(v);
    }

    llvm::GenericValue& peek() {
        ASSERTC(!value_stack.empty());
        return value_stack.top().first;
    }

    llvm::Type* peekType() {
        ASSERTC(!value_stack.empty());
        return value_stack.top().second;
    }

    llvm::Value* resolve(const std::string& name) {
        TRACE_FUNC;
        TRACE_PARAM(name);
        if(auto v = ST->getLocalValue(name))
            return const_cast<llvm::Value*>(v);
        return const_cast<llvm::Value*>(ST->getGlobalValue(name));
    }
};

AnnotationExecutor::AnnotationExecutor(FactoryNest FN, SlotTracker* ST, ExecutionEngine* ee):
    Transformer<AnnotationExecutor>{FN},
    pimpl_{ std::make_unique<Impl>(ST, ee) }{};


AnnotationExecutor::~AnnotationExecutor() {}

Annotation::Ptr AnnotationExecutor::transformAssertAnnotation(AssertAnnotationPtr a) {
    TRACE_FUNC;
    TRACE_PARAM(*a);
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{ util::toString(*a) };
    return a;
}
Annotation::Ptr AnnotationExecutor::transformAssumeAnnotation(AssumeAnnotationPtr a) {
    TRACE_FUNC;
    TRACE_PARAM(*a);
    try {
        transform(a->getTerm());
        if(!pimpl_->peek().IntVal) throw assertion_failed{ util::toString(*a) };
    } catch(assertion_failed&) {
        throw;
    } catch(std::exception& ex) {
        errs() << "Error acquired while checking assumption: " << *a << endl;
    }

    return a;
}
Annotation::Ptr AnnotationExecutor::transformRequiresAnnotation(RequiresAnnotationPtr a) {
    TRACE_FUNC;
    TRACE_PARAM(*a);
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{ util::toString(*a) };
    return a;
}
Annotation::Ptr AnnotationExecutor::transformEnsuresAnnotation(EnsuresAnnotationPtr a) {
    TRACE_FUNC;
    TRACE_PARAM(*a);
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{ util::toString(*a) };
    return a;
}

Term::Ptr AnnotationExecutor::transformValueTerm(ValueTermPtr t) {
    TRACE_FUNC;
    auto val = pimpl_->resolve(t->getVName());
    auto ret = pimpl_->ee->getOperandValue(val, pimpl_->ee->getCurrentContext());

    pimpl_->value_stack.push({ ret, val->getType() });

    return t;
}

Term::Ptr AnnotationExecutor::transformReturnValueTerm(ReturnValueTermPtr t) {
    TRACE_FUNC;
    auto term = pimpl_->ee->getCurrentContext().CurBB->getTerminator();
    llvm::Value* rval = nullptr;
    if(llvm::ReturnInst* rinst = llvm::dyn_cast<llvm::ReturnInst>(term)) {
        rval = rinst->getReturnValue();
    } else {
        UNREACHABLE("Annotation placement implies no return value");
    }
    auto ret = pimpl_->ee->getOperandValue(rval, pimpl_->ee->getCurrentContext());
    pimpl_->value_stack.push({ret, rval->getType()});
    return t;
}

Term::Ptr AnnotationExecutor::transformReturnPtrTerm(ReturnPtrTermPtr t) {
    TRACE_FUNC;
    auto term = pimpl_->ee->getCurrentContext().CurBB->getTerminator();
    llvm::Value* rval = nullptr;
    if(llvm::ReturnInst* rinst = llvm::dyn_cast<llvm::ReturnInst>(term)) {
        if(auto&& load = dyn_cast<llvm::LoadInst>(rinst->getReturnValue())) {
            rval = load->getPointerOperand();
        } else UNREACHABLE("Cannot bind return pointer to any pointer value");
    } else {
        UNREACHABLE("Annotation placement implies no return value");
    }
    auto ret = pimpl_->ee->getOperandValue(rval, pimpl_->ee->getCurrentContext());
    pimpl_->value_stack.push({ret, rval->getType()});
    return t;
}

Term::Ptr AnnotationExecutor::transformArgumentTerm(ArgumentTermPtr t) {
    TRACE_FUNC;
    auto arglist = util::viewContainer(pimpl_->ee->getCurrentContext().CurFunction->getArgumentList()).map(LAM(x,&x)).toVector();
    auto op = arglist.at(t->getIdx());
    auto ret = pimpl_->ee->getOperandValue(op, pimpl_->ee->getCurrentContext());

    pimpl_->value_stack.push({ret, op->getType()});
    return t;
}

Term::Ptr AnnotationExecutor::transformUnaryTerm(UnaryTermPtr t) {
    TRACE_FUNC;

    auto op = t->getOpcode();
    auto type = pimpl_->peekType();
    auto val = pimpl_->pop();

    llvm::GenericValue ret;

    switch(op) {
    case llvm::UnaryArithType::BNOT:
        ret.IntVal = ~val.IntVal;
        break;
    case llvm::UnaryArithType::NOT:
        ret.IntVal = !val.IntVal;
        type = llvm::Type::getInt1Ty(pimpl_->getLLVMContext());
        break;
    case llvm::UnaryArithType::NEG:
        ret.IntVal = -val.IntVal;
        break;
    }

    pimpl_->value_stack.push({ret, type});

    return t;
}
Term::Ptr AnnotationExecutor::transformBinaryTerm(BinaryTermPtr t) {
    TRACE_FUNC;

    auto rtype = pimpl_->peekType();
    auto right = pimpl_->pop();
    auto ltype = pimpl_->peekType();
    auto left = pimpl_->pop();

    llvm::Instruction::BinaryOps opc;
#define MAP_OPCODES(OURS, THEIRS) case llvm::ArithType::OURS: opc = llvm::Instruction::BinaryOps::THEIRS; break;
    switch(t->getOpcode()) {
    MAP_OPCODES(ADD, Add);
    MAP_OPCODES(SUB, Sub);
    MAP_OPCODES(MUL, Mul);
    MAP_OPCODES(DIV, SDiv);
    MAP_OPCODES(REM, SRem);
    MAP_OPCODES(LAND, And);
    MAP_OPCODES(LOR, Or);
    MAP_OPCODES(BAND, And);
    MAP_OPCODES(BOR, Or);
    MAP_OPCODES(XOR, Xor);
    MAP_OPCODES(SHL, Shl);
    MAP_OPCODES(ASHR, AShr);
    MAP_OPCODES(LSHR, LShr);
    default: break;
    }
#undef MAP_OPCODES

    auto ret_type = mergeTypes(ltype, rtype, pimpl_->ee->getDataLayout());
    adjustToType(right, rtype, ret_type);
    adjustToType(left, ltype, ret_type);

    if(t->getOpcode() == llvm::ArithType::IMPLIES) {
        if(left.IntVal.getBoolValue()) {
            pimpl_->value_stack.push({right, ret_type});
        } else {
            llvm::GenericValue RetVal;
            RetVal.IntVal = llvm::APInt{1,1};
            pimpl_->value_stack.push({RetVal, ret_type});
        };
    } else {
        auto res = pimpl_->ee->executeBinary(opc, left, right, ret_type);
        pimpl_->value_stack.push({res, ret_type});
    }

    return t;
}
Term::Ptr AnnotationExecutor::transformCmpTerm(CmpTermPtr t) {
    TRACE_FUNC;

    auto rtype = pimpl_->peekType();
    auto right = pimpl_->pop();
    auto ltype = pimpl_->peekType();
    auto left = pimpl_->pop();

    auto merged = mergeTypes(ltype, rtype, pimpl_->ee->getDataLayout());
    auto boolType = llvm::Type::getInt1Ty(pimpl_->getLLVMContext());

    auto opcode = t->getOpcode();
    llvm::GenericValue RetVal;
    if(opcode == llvm::ConditionType::TRUE) {
        RetVal.IntVal = llvm::APInt(1, 1);
        pimpl_->value_stack.push({ RetVal, boolType });
        return t;
    }
    if(opcode == llvm::ConditionType::FALSE) {
        RetVal.IntVal = llvm::APInt(1, 0);
        pimpl_->value_stack.push({ RetVal, boolType });
        return t;
    }
    unsigned executiveOpcode = llvm::ICmpInst::BAD_ICMP_PREDICATE;

    if(merged->isFloatingPointTy()) {
        switch(opcode) {
        case llvm::ConditionType::EQ        : executiveOpcode = llvm::FCmpInst::FCMP_OEQ; break;
        case llvm::ConditionType::NEQ       : executiveOpcode = llvm::FCmpInst::FCMP_ONE; break;
        case llvm::ConditionType::GT        : executiveOpcode = llvm::FCmpInst::FCMP_OGT; break;
        case llvm::ConditionType::GE        : executiveOpcode = llvm::FCmpInst::FCMP_OGE; break;
        case llvm::ConditionType::LT        : executiveOpcode = llvm::FCmpInst::FCMP_OLT; break;
        case llvm::ConditionType::LE        : executiveOpcode = llvm::FCmpInst::FCMP_OLE; break;
        case llvm::ConditionType::UGT       : executiveOpcode = llvm::FCmpInst::FCMP_UGT; break;
        case llvm::ConditionType::UGE       : executiveOpcode = llvm::FCmpInst::FCMP_UGE; break;
        case llvm::ConditionType::ULT       : executiveOpcode = llvm::FCmpInst::FCMP_ULT; break;
        case llvm::ConditionType::ULE       : executiveOpcode = llvm::FCmpInst::FCMP_ULE; break;
        default                             : UNREACHABLE("illegal opcode");
        }
    } else {
        switch(opcode) {
        case llvm::ConditionType::EQ        : executiveOpcode = llvm::ICmpInst::ICMP_EQ; break;
        case llvm::ConditionType::NEQ       : executiveOpcode = llvm::ICmpInst::ICMP_NE; break;
        case llvm::ConditionType::GT        : executiveOpcode = llvm::ICmpInst::ICMP_SGT; break;
        case llvm::ConditionType::GE        : executiveOpcode = llvm::ICmpInst::ICMP_SGE; break;
        case llvm::ConditionType::LT        : executiveOpcode = llvm::ICmpInst::ICMP_SLT; break;
        case llvm::ConditionType::LE        : executiveOpcode = llvm::ICmpInst::ICMP_SLE; break;
        case llvm::ConditionType::UGT       : executiveOpcode = llvm::ICmpInst::ICMP_UGT; break;
        case llvm::ConditionType::UGE       : executiveOpcode = llvm::ICmpInst::ICMP_UGE; break;
        case llvm::ConditionType::ULT       : executiveOpcode = llvm::ICmpInst::ICMP_ULT; break;
        case llvm::ConditionType::ULE       : executiveOpcode = llvm::ICmpInst::ICMP_ULE; break;
        default                             : UNREACHABLE("illegal opcode");
        }
    }

    adjustToType(right, rtype, merged);
    adjustToType(left, ltype, merged);

    auto ret = pimpl_->ee->executeCmpInst(executiveOpcode, left, right, merged);

    pimpl_->value_stack.push({ ret, boolType });

    return t;
}
Term::Ptr AnnotationExecutor::transformTernaryTerm(TernaryTermPtr t) {
    TRACE_FUNC;

    auto ftype = pimpl_->peekType();
    auto fls   = pimpl_->pop();
    auto ttype = pimpl_->peekType();
    auto tru   = pimpl_->pop();
    auto cond  = pimpl_->pop();

    auto ret_type = mergeTypes(ttype, ftype, pimpl_->ee->getDataLayout());
    adjustToType(tru, ttype, ret_type);
    adjustToType(fls, ftype, ret_type);

    if(cond.IntVal.getBoolValue()) pimpl_->value_stack.push({tru, ret_type});
    else pimpl_->value_stack.push({fls, ret_type});
    return t;
}
Term::Ptr AnnotationExecutor::transformSignTerm(SignTermPtr t) {
    TRACE_FUNC;

    auto v = pimpl_->pop();
    llvm::GenericValue res;
    res.IntVal = llvm::APInt(1, v.IntVal.isNonNegative());
    pimpl_->value_stack.push({res, llvm::Type::getInt1Ty(pimpl_->getLLVMContext())});
    return t;
}

Term::Ptr AnnotationExecutor::transformGepTerm(GepTermPtr t) {
    TRACE_FUNC;

    std::vector<llvm::GenericValue> shifts;
    for(auto&& shift: t->getShifts()) {
        util::use(shift);
        shifts.push_back(pimpl_->pop());
    }
    std::reverse(std::begin(shifts), std::end(shifts));

    auto base_t = pimpl_->peekType();
    auto base = pimpl_->pop();

    auto vShifts =
        util::viewContainer(shifts)
        .map(LAM(x, (llvm::Value*)llvm::ConstantInt::get(base_t->getContext(), x.IntVal)))
        .toVector();
    auto gep_begin = llvm::gep_type_begin(base_t, llvm::ArrayRef<llvm::Value*>(vShifts));
    auto gep_end = llvm::gep_type_end(base_t, llvm::ArrayRef<llvm::Value*>(vShifts));

    auto DL = pimpl_->ee->getDataLayout();

    uint64_t Total = 0;

    llvm::Type* lastType = base_t;
    for(auto&& TyIxPair : util::view(gep_begin, gep_end) ^ util::viewContainer(shifts)) {
        if (llvm::StructType *STy = llvm::dyn_cast<llvm::StructType>(TyIxPair.first)) {
            const llvm::StructLayout *SLO = DL->getStructLayout(STy);

            auto Ix = TyIxPair.second.IntVal.getZExtValue();
            Total += SLO->getElementOffset(Ix);
            lastType = STy->getElementType(Ix);
        } else {
            llvm::SequentialType *ST = llvm::cast<llvm::SequentialType>(TyIxPair.first);
            // Get the index number for the array... which must be long llvm::Type...
            llvm::GenericValue IdxGV = TyIxPair.second;

            Total += IdxGV.IntVal.getZExtValue() * DL->getTypeAllocSize(ST->getElementType());
            lastType = ST->getElementType();
        }
    }

    uint8_t* byteBase = static_cast<uint8_t*>(base.PointerVal);
    byteBase += Total;
    base.PointerVal = byteBase;

    pimpl_->value_stack.push({ base, llvm::PointerType::get(lastType, 0) });

    /// FIXME: THINK!
    return t;
}
Term::Ptr AnnotationExecutor::transformLoadTerm(LoadTermPtr t) {
    TRACE_FUNC;

    auto pttype = pimpl_->peekType();
    auto ptr = pimpl_->pop();

    llvm::GenericValue res;

    if(ptr.PointerVal == pimpl_->ee->getSymbolicPointer()) {
        throw std::logic_error("Cannot load a symbolic pointer in annotation context");
    }

    pimpl_->ee->LoadValueFromMemory(
        res,
        static_cast<const uint8_t*>(ptr.PointerVal),
        pttype->getPointerElementType()
    );

    pimpl_->value_stack.push({res, pttype->getPointerElementType()});
    return t;
}

Term::Ptr AnnotationExecutor::transformAxiomTerm(AxiomTermPtr t) {
    TRACE_FUNC;

    auto axiom = pimpl_->pop();

    if(!axiom.IntVal) throw assertion_failed{ t->getName() };

    // do nothing - value is already on the stack's top

    return t;
}

Term::Ptr AnnotationExecutor::transformOpaqueUndefTerm(OpaqueUndefTermPtr t) {
    TRACE_FUNC;
    auto ret_type = TypeUtils::tryCastBack(pimpl_->getLLVMContext(), t->getType());
    llvm::GenericValue res =
        pimpl_->ee->getOperandValue(
            llvm::UndefValue::get(ret_type),
            pimpl_->ee->getCurrentContext()
        );

    pimpl_->value_stack.push({res, ret_type});
    return t;

}

Term::Ptr AnnotationExecutor::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr t) {
    TRACE_FUNC;

    llvm::GenericValue res;
    res.PointerVal = 0;
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->getLLVMContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr t) {
    TRACE_FUNC;

    llvm::GenericValue res;
    res.PointerVal = 0; // FIXME: think!
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->getLLVMContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr t) {
    TRACE_FUNC;

    llvm::GenericValue res;
    res.IntVal = llvm::APInt(64, t->getValue());
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->getLLVMContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr t) {
    TRACE_FUNC;

    llvm::GenericValue res;
    res.DoubleVal = t->getValue();
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->getLLVMContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr t) {
    TRACE_FUNC;

    llvm::GenericValue res;
    res.IntVal = llvm::APInt(1, t->getValue());
    pimpl_->value_stack.push({res, llvm::Type::getInt1Ty(pimpl_->getLLVMContext())});
    return t;
}


} /* namespace borealis */

#include "Util/unmacros.h"
