#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>

#include "Executor/AnnotationExecutor.h"
#include "Executor/ExecutionEngine.h"
#include "Executor/Exceptions.h"

#include "Util/macros.h"

namespace borealis {

static llvm::Type* mergeTypes(llvm::Type* lhv, llvm::Type* rhv, const llvm::DataLayout* dl) {
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

    throw std::logic_error("unmergeable types");
}

static void adjustToType(llvm::GenericValue& v, llvm::Type* pastType, llvm::Type* type) {
    if(pastType == type) return;
    if(type->isIntegerTy()) {
        auto sz = type->getIntegerBitWidth();
        if(v.IntVal.getBitWidth() != sz)
            v.IntVal = v.IntVal.sext(sz);
    } else if(type->isDoubleTy() && pastType->isFloatTy()) {
        v.DoubleVal = v.FloatVal;
    }   else if(type->isFloatTy() && pastType->isDoubleTy()) {
        v.FloatVal = v.DoubleVal;
    }
}

struct AntiSlotTracker {
    std::unordered_map<std::string, llvm::Value*> vals;

    AntiSlotTracker(SlotTracker* st, llvm::Module* M) {
        auto funcsView = util::viewContainer(*M).filter(LAM(F, !F.isDeclaration()));

        for(auto&& i : funcsView.flatten().flatten()) {
            vals[st->getLocalName(&i)] = &i;
        }
    }
};

struct AnnotationExecutor::Impl {
    llvm::Module* M;
    ExecutionEngine* ee;
    SlotTracker* st;
    AntiSlotTracker ast;
    std::stack<std::pair<llvm::GenericValue, llvm::Type*>> value_stack;

    Impl(llvm::Module* M, SlotTracker* st, ExecutionEngine* ee): M{M}, st{st}, ast{st, M}, ee{ee} {};

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
};

AnnotationExecutor::AnnotationExecutor(FactoryNest FN, llvm::Module* M, SlotTracker* st, ExecutionEngine* ee):
    Transformer<AnnotationExecutor>{FN},
    pimpl_{ std::make_unique<Impl>(M, st, ee) }{};
AnnotationExecutor::~AnnotationExecutor() {}

Annotation::Ptr AnnotationExecutor::transformAssertAnnotation(AssertAnnotationPtr a) {
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{};
    return a;
}
Annotation::Ptr AnnotationExecutor::transformAssumeAnnotation(AssumeAnnotationPtr a) {
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{};
    return a;
}
Annotation::Ptr AnnotationExecutor::transformRequiresAnnotation(RequiresAnnotationPtr a) {
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{};
    return a;
}
Annotation::Ptr AnnotationExecutor::transformEnsuresAnnotation(EnsuresAnnotationPtr a) {
    transform(a->getTerm());
    if(!pimpl_->peek().IntVal) throw assertion_failed{};
    return a;
}

Term::Ptr AnnotationExecutor::transformValueTerm(ValueTermPtr t) {
    auto val = pimpl_->ast.vals.at(t->getName());
    auto ret = pimpl_->ee->getOperandValue(val, pimpl_->ee->getCurrentContext());

    pimpl_->value_stack.push({ ret, val->getType() });

    return t;
}

Term::Ptr AnnotationExecutor::transformReturnValueTerm(ReturnValueTermPtr t) {
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
Term::Ptr AnnotationExecutor::transformArgumentTerm(ArgumentTermPtr t) {
    auto op = pimpl_->ee->getCurrentContext().CurFunction->getOperand(t->getIdx());
    auto ret = pimpl_->ee->getOperandValue(op, pimpl_->ee->getCurrentContext());

    pimpl_->value_stack.push({ret, op->getType()});
    return t;
}

Term::Ptr AnnotationExecutor::transformUnaryTerm(UnaryTermPtr t) {

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
        type = llvm::Type::getInt1Ty(pimpl_->M->getContext());
        break;
    case llvm::UnaryArithType::NEG:
        ret.IntVal = -val.IntVal;
        break;
    }

    pimpl_->value_stack.push({ret, type});

    return t;
}
Term::Ptr AnnotationExecutor::transformBinaryTerm(BinaryTermPtr t) {
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
    }
#undef MAP_OPCODES

    auto ret_type = mergeTypes(ltype, rtype, pimpl_->ee->getDataLayout());
    adjustToType(right, rtype, ret_type);
    adjustToType(left, ltype, ret_type);

    auto res = pimpl_->ee->executeBinary(opc, left, right, ret_type);
    pimpl_->value_stack.push({res, ret_type});

    return t;
}
Term::Ptr AnnotationExecutor::transformCmpTerm(CmpTermPtr t) {

    auto rtype = pimpl_->peekType();
    auto right = pimpl_->pop();
    auto ltype = pimpl_->peekType();
    auto left = pimpl_->pop();

    auto merged = mergeTypes(ltype, rtype, pimpl_->ee->getDataLayout());
    auto boolType = llvm::Type::getInt1Ty(pimpl_->M->getContext());

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
    unsigned executiveOpcode;

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
        case llvm::ConditionType::UNKNOWN   : UNREACHABLE("unknown opcode");
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
        case llvm::ConditionType::UNKNOWN   : UNREACHABLE("unknown opcode");
        }
    }

    adjustToType(right, rtype, merged);
    adjustToType(left, ltype, merged);

    auto ret = pimpl_->ee->executeCmpInst(executiveOpcode, left, right, merged);

    pimpl_->value_stack.push({ ret, boolType });

    return t;
}
Term::Ptr AnnotationExecutor::transformTernaryTerm(TernaryTermPtr t) {
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
    auto v = pimpl_->pop();
    llvm::GenericValue res;
    res.IntVal = llvm::APInt(1, v.IntVal.isNonNegative());
    pimpl_->value_stack.push({res, llvm::Type::getInt1Ty(pimpl_->M->getContext())});
    return t;
}

Term::Ptr AnnotationExecutor::transformGepTerm(GepTermPtr t) {
    std::vector<llvm::GenericValue> shifts;
    for(auto&& shift: t->getShifts()) {
        shifts.push_back(pimpl_->pop());
    }
    std::reverse(std::begin(shifts), std::end(shifts));

    auto base_t = pimpl_->peekType();
    auto base = pimpl_->pop();

    std::vector<llvm::Value*> pseudoStuff(shifts.size(), nullptr);
    auto gep_begin = llvm::gep_type_begin(base_t, llvm::ArrayRef<llvm::Value*>(pseudoStuff));
    auto gep_end = llvm::gep_type_end(base_t, llvm::ArrayRef<llvm::Value*>(pseudoStuff));

    auto DL = pimpl_->ee->getDataLayout();

    uint64_t Total = 0;

    llvm::Type* lastType = base_t;
    for(auto&& TyIxPair : util::view(gep_begin, gep_end) ^ util::viewContainer(shifts)) {
        if (llvm::StructType *STy = llvm::dyn_cast<llvm::StructType>(TyIxPair.first)) {
            const llvm::StructLayout *SLO = DL->getStructLayout(STy);

            Total += SLO->getElementOffset(TyIxPair.second.IntVal.getZExtValue());
        } else {
            llvm::SequentialType *ST = llvm::cast<llvm::SequentialType>(TyIxPair.first);
            // Get the index number for the array... which must be long llvm::Type...
            llvm::GenericValue IdxGV = TyIxPair.second;

            Total += IdxGV.IntVal.getZExtValue() * DL->getTypeAllocSize(ST->getElementType());
        }
        lastType = TyIxPair.first;
    }

    uint8_t* byteBase = static_cast<uint8_t*>(base.PointerVal);
    byteBase += Total;
    base.PointerVal = byteBase;

    pimpl_->value_stack.push({ base, lastType });

    /// FIXME: THINK!
    return t;
}
Term::Ptr AnnotationExecutor::transformLoadTerm(LoadTermPtr t) {
    auto pttype = pimpl_->peekType();
    auto ptr = pimpl_->pop();

    llvm::GenericValue res;

    pimpl_->ee->LoadValueFromMemory(
        res,
        static_cast<const uint8_t*>(ptr.PointerVal),
        pttype->getPointerElementType()
    );

    pimpl_->value_stack.push({res, pttype->getPointerElementType()});
    return t;
}

Term::Ptr AnnotationExecutor::transformAxiomTerm(AxiomTermPtr t) {
    auto axiom = pimpl_->pop();

    if(!axiom.IntVal) throw assertion_failed{};

    // do nothing - value is already on the stack's top

    return t;
}

Term::Ptr AnnotationExecutor::transformOpaqueNullPtrTerm(OpaqueNullPtrTermPtr t) {
    llvm::GenericValue res;
    res.PointerVal = 0;
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->M->getContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueInvalidPtrTerm(OpaqueInvalidPtrTermPtr t) {
    llvm::GenericValue res;
    res.PointerVal = 0; // FIXME: think!
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->M->getContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueIntConstantTerm(OpaqueIntConstantTermPtr t) {
    llvm::GenericValue res;
    res.IntVal = llvm::APInt(64, t->getValue());
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->M->getContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueFloatingConstantTerm(OpaqueFloatingConstantTermPtr t) {
    llvm::GenericValue res;
    res.DoubleVal = t->getValue();
    pimpl_->value_stack.push({res, llvm::Type::getVoidTy(pimpl_->M->getContext())});
    return t;
}
Term::Ptr AnnotationExecutor::transformOpaqueBoolConstantTerm(OpaqueBoolConstantTermPtr t) {
    llvm::GenericValue res;
    res.IntVal = llvm::APInt(1, t->getValue());
    pimpl_->value_stack.push({res, llvm::Type::getInt1Ty(pimpl_->M->getContext())});
    return t;
}


} /* namespace borealis */

#include "Util/unmacros.h"
