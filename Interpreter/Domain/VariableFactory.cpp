//
// Created by abdullin on 2/11/19.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "GlobalManager.hpp"
#include "VariableFactory.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

Type::Ptr VariableFactory::cast(const llvm::Type* type) const {
    auto res = af_->tf()->cast(type, dl_);
    if (llvm::isa<type::Bool>(res)) return af_->tf()->getInteger(1);
    else return res;
}

AbstractDomain::Ptr VariableFactory::top(Type::Ptr type) const {
    return af_->top(type);
}

AbstractDomain::Ptr VariableFactory::bottom(Type::Ptr type) const {
    return af_->bottom(type);
}

AbstractDomain::Ptr VariableFactory::get(const llvm::Value* val) const {
    auto&& type = cast(val->getType());
    if (llvm::isa<type::UnknownType>(type.get())) {
        return nullptr;
    } else if (llvm::isa<type::Bool>(type.get())) {
        return af_->getBool(AbstractFactory::BOTTOM);
    } else if (llvm::isa<type::Integer>(type.get())) {
        return af_->getInteger(type, AbstractFactory::BOTTOM);
    } else if (llvm::isa<type::Float>(type.get())) {
        return af_->getFloat(AbstractFactory::BOTTOM);
    } else if (llvm::isa<type::Array>(type.get())) {
        return af_->getArray(type);
    } else if (llvm::isa<type::Record>(type.get())) {
        return af_->getStruct(type);
    } else if (llvm::isa<type::Function>(type.get())) {
        auto&& ptr = af_->tf()->getPointer(type, af_->defaultSize);
        return af_->getPointer(ptr);
    } else if (llvm::isa<type::Pointer>(type.get())) {
        return af_->getPointer(type);
    } else {
        errs() << "Creating domain of unknown type <" << type << ">" << endl;
        return nullptr;
    }
}

AbstractDomain::Ptr VariableFactory::get(const llvm::Constant* constant) const {
    // Integer
    if (auto intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return af_->getInteger(*intConstant->getValue().getRawData(), intConstant->getBitWidth());
        // Float
    } else if (auto floatConstant = llvm::dyn_cast<llvm::ConstantFP>(constant)) {
        return af_->getFloat(floatConstant->getValueAPF().convertToDouble());
        // Null pointer
    } else if (llvm::isa<llvm::ConstantPointerNull>(constant)) {
        return af_->getNullptr(cast(constant->getType()));
        // Constant Data Array
    } else if (auto sequential = llvm::dyn_cast<llvm::ConstantDataSequential>(constant)) {
        std::vector<AbstractDomain::Ptr> elements;
        for (auto i = 0U; i < sequential->getNumElements(); ++i) {
            auto element = get(sequential->getElementAsConstant(i));
            if (not element) {
                element = af_->top(cast(sequential->getType()));
            }
            elements.emplace_back(element);
        }
        return af_->getArray(cast(constant->getType()), elements);
        // Constant Array
    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(constant)) {
        std::vector<AbstractDomain::Ptr> elements;
        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
            auto element = get(constantArray->getOperand(i));
            if (not element) {
                element = af_->top(cast(sequential->getType()));
            }
            elements.emplace_back(element);
        }
        return af_->getArray(cast(constant->getType()), elements);
        // Constant Expr
    } else if (auto constExpr = llvm::dyn_cast<llvm::ConstantExpr>(constant)) {
//        return interpretConstantExpr(constExpr);
        return af_->top(cast(constExpr->getType()));
        // Constant Struct
    } else if (auto structType = llvm::dyn_cast<llvm::ConstantStruct>(constant)) {
        std::vector<AbstractDomain::Ptr> elements;
        for (auto i = 0U; i < structType->getNumOperands(); ++i) {
            auto element = get(structType->getAggregateElement(i));
            if (not element) {
                element = af_->top(cast(sequential->getType()));
            }
            elements.emplace_back(element);
        }
        return af_->getStruct(cast(constant->getType()), elements);
        // Function
    } else if (auto function = llvm::dyn_cast<llvm::Function>(constant)) {
        return af_->getPointer(cast(function->getType()));
        // Zero initializer
    } else if (llvm::isa<llvm::ConstantAggregateZero>(constant)) {
        return af_->top(cast(constant->getType()));
        // Undef
    } else if (llvm::isa<llvm::UndefValue>(constant)) {
        return af_->bottom(cast(constant->getType()));
        // otherwise
    } else {
        return af_->top(cast(constant->getType()));
    }
}

AbstractDomain::Ptr VariableFactory::get(const llvm::GlobalVariable* global) const {
    AbstractDomain::Ptr globalDomain;
    if (global->hasInitializer()) {
        AbstractDomain::Ptr content;
        auto& elementType = *global->getType()->getPointerElementType();
        // If global is simple type, that we should wrap it like array of size 1
        auto ptrType = cast(global->getType());
        if (elementType.isIntegerTy() || elementType.isFloatingPointTy()) {
            auto simpleConst = get(global->getInitializer());
            if (simpleConst->isTop()) return af_->top(cast(global->getType()));
            auto&& arrayType = af_->tf()->getArray(cast(&elementType), 1);
            content = af_->getArray(arrayType, { simpleConst });
            ptrType = af_->tf()->getPointer(arrayType, af_->defaultSize);
            // else just create domain
        } else {
            content = get(global->getInitializer());
            if (content->isTop()) return af_->top(cast(global->getType()));
        }
        ASSERT(content, "Unsupported constant");

        auto ptr = af_->getPointer(ptrType, content, af_->getMachineInt(0));

        // we need this because GEPs for global structs and arrays contain one additional index at the start
        if (not (elementType.isIntegerTy() || elementType.isFloatingPointTy())) {
            auto newArray = af_->tf()->getArray(cast(global->getType()), 1);
            auto newLevel = af_->getArray(newArray, { ptr });
            globalDomain = af_->getPointer(newArray, newLevel, af_->getMachineInt(0));
        } else {
            globalDomain = ptr;
        }

    } else {
        globalDomain = af_->bottom(cast(global->getType()));
    }

    ASSERTC(globalDomain);
    return globalDomain;
}


AbstractDomain::Ptr VariableFactory::getConstOperand(const llvm::Constant* c) const {
    if (llvm::isa<llvm::GlobalVariable>(c)) return gm_->findGlobal(c);
    else return get(c);
}


#define HANDLE_BINOP(EXPR) \
    lhv = getConstOperand(ce->getOperand(0)); \
    rhv = getConstOperand(ce->getOperand(1)); \
    ASSERT(lhv && rhv, "Unknown binary constexpr operands"); \
    return (EXPR);

#define HANDLE_CAST(OPER) \
    lhv = getConstOperand(ce->getOperand(0)); \
    ASSERT(lhv, "Unknown cast constexpr operand"); \
    return af_->top(cast(ce->getType()));

CmpOperator toCmp(llvm::CmpInst::Predicate p) {
    switch (p) {
        case llvm::CmpInst::FCMP_FALSE: return CmpOperator::FALSE;
        case llvm::CmpInst::FCMP_OEQ: return CmpOperator::EQ;
        case llvm::CmpInst::FCMP_OGT: return CmpOperator::GT;
        case llvm::CmpInst::FCMP_OGE: return CmpOperator::GE;
        case llvm::CmpInst::FCMP_OLT: return CmpOperator::LT;
        case llvm::CmpInst::FCMP_OLE: return CmpOperator::LE;
        case llvm::CmpInst::FCMP_ONE: return CmpOperator::NEQ;
        case llvm::CmpInst::FCMP_ORD: return CmpOperator::TOP;
        case llvm::CmpInst::FCMP_UNO: return CmpOperator::TOP;
        case llvm::CmpInst::FCMP_UEQ: return CmpOperator::EQ;
        case llvm::CmpInst::FCMP_UGT: return CmpOperator::GT;
        case llvm::CmpInst::FCMP_UGE: return CmpOperator::GE;
        case llvm::CmpInst::FCMP_ULT: return CmpOperator::LT;
        case llvm::CmpInst::FCMP_ULE: return CmpOperator::LE;
        case llvm::CmpInst::FCMP_UNE: return CmpOperator::NEQ;
        case llvm::CmpInst::FCMP_TRUE: return CmpOperator::TRUE;
        case llvm::CmpInst::ICMP_EQ: return CmpOperator::EQ;
        case llvm::CmpInst::ICMP_NE: return CmpOperator::NEQ;
        case llvm::CmpInst::ICMP_UGT: return CmpOperator::GT;
        case llvm::CmpInst::ICMP_UGE: return CmpOperator::GE;
        case llvm::CmpInst::ICMP_ULT: return CmpOperator::LT;
        case llvm::CmpInst::ICMP_ULE: return CmpOperator::LE;
        case llvm::CmpInst::ICMP_SGT: return CmpOperator::GT;
        case llvm::CmpInst::ICMP_SGE: return CmpOperator::GE;
        case llvm::CmpInst::ICMP_SLT: return CmpOperator::LT;
        case llvm::CmpInst::ICMP_SLE: return CmpOperator::LE;
        default:
            UNREACHABLE("Unknown predicete");
    }
}

AbstractDomain::Ptr VariableFactory::interpretConstantExpr(const llvm::ConstantExpr* ce) const {
    AbstractDomain::Ptr lhv, rhv;
    switch (ce->getOpcode()) {
        case llvm::Instruction::Add: HANDLE_BINOP((lhv + rhv));
        case llvm::Instruction::FAdd: HANDLE_BINOP((lhv + rhv));
        case llvm::Instruction::Sub: HANDLE_BINOP((lhv - rhv));
        case llvm::Instruction::FSub: HANDLE_BINOP((lhv - rhv));
        case llvm::Instruction::Mul: HANDLE_BINOP((lhv * rhv));
        case llvm::Instruction::FMul: HANDLE_BINOP((lhv * rhv));
        case llvm::Instruction::UDiv: HANDLE_BINOP((lhv / rhv));
        case llvm::Instruction::SDiv: HANDLE_BINOP((lhv / rhv));
        case llvm::Instruction::FDiv: HANDLE_BINOP((lhv / rhv));
        case llvm::Instruction::URem: HANDLE_BINOP((lhv % rhv));
        case llvm::Instruction::SRem: HANDLE_BINOP((lhv % rhv));
        case llvm::Instruction::FRem: HANDLE_BINOP((lhv % rhv));
        case llvm::Instruction::And: HANDLE_BINOP((lhv & rhv));
        case llvm::Instruction::Or: HANDLE_BINOP((lhv | rhv));
        case llvm::Instruction::Xor: HANDLE_BINOP((lhv ^ rhv));
        case llvm::Instruction::Shl: HANDLE_BINOP((lhv << rhv));
        case llvm::Instruction::LShr: HANDLE_BINOP((lshr(lhv, rhv)));
        case llvm::Instruction::AShr: HANDLE_BINOP((lhv >> rhv));
        case llvm::Instruction::ICmp: HANDLE_BINOP((lhv->apply(toCmp(static_cast<llvm::CmpInst::Predicate>(ce->getPredicate())), rhv)));
        case llvm::Instruction::FCmp: HANDLE_BINOP((lhv->apply(toCmp(static_cast<llvm::CmpInst::Predicate>(ce->getPredicate())), rhv)));
        case llvm::Instruction::Trunc: HANDLE_CAST(trunc);
        case llvm::Instruction::SExt: HANDLE_CAST(sext);
        case llvm::Instruction::ZExt: HANDLE_CAST(zext);
        case llvm::Instruction::FPTrunc: HANDLE_CAST(fptrunc);
        case llvm::Instruction::UIToFP: HANDLE_CAST(uitofp);
        case llvm::Instruction::SIToFP: HANDLE_CAST(sitofp);
        case llvm::Instruction::FPToUI: HANDLE_CAST(fptoui);
        case llvm::Instruction::FPToSI: HANDLE_CAST(fptosi);
        case llvm::Instruction::PtrToInt: HANDLE_CAST(ptrtoint);
        case llvm::Instruction::IntToPtr: HANDLE_CAST(inttoptr);
        case llvm::Instruction::BitCast: HANDLE_CAST(bitcast);
        case llvm::Instruction::GetElementPtr:
            return handleGEPConstantExpr(ce);
        default:
            UNREACHABLE("Unknown opcode of ConstExpr");
    }
}

#undef HANDLE_BINOP
#undef HANDLE_CAST

AbstractDomain::Ptr VariableFactory::handleGEPConstantExpr(const llvm::ConstantExpr* ce) const {
    auto&& ptr = getConstOperand(ce->getOperand(0));
    ASSERT(ptr, "gep args");

    std::vector<AbstractDomain::Ptr> offsets;
    for (auto j = 1U; j != ce->getNumOperands(); ++j) {
        auto val = ce->getOperand(j);
        if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(val)) {
            offsets.emplace_back(af_->getMachineInt(*intConstant->getValue().getRawData()));
        } else if (auto indx = getConstOperand(val)) {
            offsets.emplace_back(indx);
        } else {
            UNREACHABLE("Non-integer constant in gep");
        }
    }
    return ptr->gep(cast(ce->getType()), offsets);
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"