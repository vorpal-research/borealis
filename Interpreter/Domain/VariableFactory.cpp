//
// Created by abdullin on 2/11/19.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "AbstractFactory.hpp"
#include "GlobalManager.hpp"
#include "VariableFactory.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {


VariableFactory::VariableFactory(const llvm::DataLayout* dl, GlobalManager* gm) :
        af_(AbstractFactory::get()), dl_(dl), gm_(gm) {}

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
        return af_->getNullptr();
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
        return interpretConstantExpr(constExpr);
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
            ptrType = af_->tf()->getPointer(arrayType, AbstractFactory::defaultSize);
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
    if (llvm::isa<llvm::GlobalVariable>(c)) return gm_->global(c);
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
    return af_->cast((OPER), cast(ce->getType()), lhv);

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
        case llvm::Instruction::ICmp: HANDLE_BINOP((lhv->apply(llvm::conditionType(ce->getPredicate()), rhv)));
        case llvm::Instruction::FCmp: HANDLE_BINOP((lhv->apply(llvm::conditionType(ce->getPredicate()), rhv)));
        case llvm::Instruction::Trunc: HANDLE_CAST(CastOperator::TRUNC);
        case llvm::Instruction::SExt: HANDLE_CAST(CastOperator::SEXT);
        case llvm::Instruction::ZExt: HANDLE_CAST(CastOperator::EXT);
        case llvm::Instruction::FPTrunc: HANDLE_CAST(CastOperator::TRUNC);
        case llvm::Instruction::UIToFP: HANDLE_CAST(CastOperator::ITOFP);
        case llvm::Instruction::SIToFP: HANDLE_CAST(CastOperator::ITOFP);
        case llvm::Instruction::FPToUI: HANDLE_CAST(CastOperator::FPTOI);
        case llvm::Instruction::FPToSI: HANDLE_CAST(CastOperator::FPTOI);
        case llvm::Instruction::PtrToInt: HANDLE_CAST(CastOperator::PTRTOI);
        case llvm::Instruction::IntToPtr: HANDLE_CAST(CastOperator::ITOPTR);
        case llvm::Instruction::BitCast: HANDLE_CAST(CastOperator::BITCAST);
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

AbstractDomain::Ptr VariableFactory::findGlobal(const llvm::Value* val) const {
    return gm_->global(val);
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"