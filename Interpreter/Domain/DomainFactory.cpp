//
// Created by abdullin on 2/7/17.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "DomainFactory.h"
#include "Interpreter/Domain/Integer/IntValue.h"
#include "Interpreter/IR/Module.h"
#include "Interpreter/Util.hpp"
#include "Util/cast.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {

DomainFactory::DomainFactory(Module* module) : ObjectLevelLogging("domain"),
                                               module_(module),
                                               ST_(module_->getSlotTracker()),
                                               int_cache_(LAM(a, Domain::Ptr{ new IntegerIntervalDomain(this, a) })),
                                               float_cache_(LAM(a, Domain::Ptr{ new FloatIntervalDomain(this, a) })),
                                               nullptr_{ new NullptrDomain(this) } {}

DomainFactory::~DomainFactory() {
    auto&& info = infos();
    info << "DomainFactory statistics:" << endl;
    info << "Integers: " << int_cache_.size() << endl;
    info << "Floats: " << float_cache_.size() << endl;
    info << endl;
}

#define GENERATE_GET_VALUE(VALUE) \
    if (type.isVoidTy()) { \
        return nullptr; \
    } else if (type.isIntegerTy()) { \
        auto&& intType = llvm::cast<llvm::IntegerType>(&type); \
        return getInteger(VALUE, intType->getBitWidth()); \
    } else if (type.isFloatingPointTy()) { \
        auto& semantics = util::getSemantics(type); \
        return getFloat(VALUE, semantics); \
    } else if (type.isAggregateType()) { \
        return getAggregate(type); \
    } else if (type.isFunctionTy()) { \
        return getFunction(type); \
    } else if (type.isPointerTy()) { \
        return getPointer(VALUE, *type.getPointerElementType()); \
    } else { \
        errs() << "Creating domain of unknown type <" << ST_->toString(&type) << ">" << endl; \
        return nullptr; \
    }


/*  General */
Domain::Ptr DomainFactory::getTop(const llvm::Type& type) {
    GENERATE_GET_VALUE(Domain::TOP);
}

Domain::Ptr DomainFactory::getBottom(const llvm::Type& type) {
    GENERATE_GET_VALUE(Domain::BOTTOM);
}

Domain::Ptr DomainFactory::get(const llvm::Value* val) {
    auto&& type = *val->getType();
    // Void type - do nothing
    if (type.isVoidTy()) {
        return nullptr;
    // Integer
    } else if (type.isIntegerTy()) {
        auto&& intType = llvm::cast<llvm::IntegerType>(&type);
        return getInteger(intType->getBitWidth());
    // Float
    } else if (type.isFloatingPointTy()) {
        auto& semantics = util::getSemantics(type);
        return getFloat(semantics);
    // Aggregate type (Array or Struct)
    } else if (type.isAggregateType()) {
        return getAggregate(type);
    // Function
    } else if (type.isFunctionTy()) {
        return getFunction(type);
    // PointerDomain
    } else if (type.isPointerTy()) {
        return allocate(type);
    // Otherwise
    } else {
        errs() << "Creating domain of unknown type <" << ST_->toString(&type) << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr DomainFactory::get(const llvm::Constant* constant) {
    // Integer
    if (auto intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return getInteger(toInteger(intConstant->getValue()));
    // Float
    } else if (auto floatConstant = llvm::dyn_cast<llvm::ConstantFP>(constant)) {
        return getFloat(llvm::APFloat(floatConstant->getValueAPF()));
    // Null pointer
    } else if (llvm::isa<llvm::ConstantPointerNull>(constant)) {
        return getNullptr(*constant->getType()->getPointerElementType());
    // Constant Data Array
    } else if (auto sequential = llvm::dyn_cast<llvm::ConstantDataSequential>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < sequential->getNumElements(); ++i) {
            auto element = get(sequential->getElementAsConstant(i));
            if (not element) {
                errs() << "Cannot create constant: " << ST_->toString(sequential->getElementAsConstant(i)) << endl;
                element = getTop(*sequential->getType());
            }
            elements.emplace_back(element);
        }
        return getAggregate(*constant->getType(), elements);
    // Constant Array
    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
            auto element = get(constantArray->getOperand(i));
            if (not element) {
                errs() << "Cannot create constant: " << ST_->toString(constantArray->getOperand(i)) << endl;
                element = getTop(*sequential->getType());
            }
            elements.emplace_back(element);
        }
        return getAggregate(*constant->getType(), elements);
    // Constant Expr
    } else if (auto constExpr = llvm::dyn_cast<llvm::ConstantExpr>(constant)) {
        return interpretConstantExpr(constExpr);
    // Constant Struct
    } else if (auto structType = llvm::dyn_cast<llvm::ConstantStruct>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < structType->getNumOperands(); ++i) {
            auto element = get(structType->getAggregateElement(i));
            if (not element) {
                errs() << "Cannot create constant: " << ST_->toString(structType->getAggregateElement(i)) << endl;
                element = getTop(*sequential->getType());
            }
            elements.emplace_back(element);
        }
        return getAggregate(*constant->getType(), elements);
    // Function
    } else if (auto function = llvm::dyn_cast<llvm::Function>(constant)) {
        auto& functype = *function->getType()->getPointerElementType();
        auto&& arrayType = llvm::ArrayType::get(function->getType(), 1);
        auto&& arrayDom = getAggregate(*arrayType, {getFunction(functype, module_->get(function))});
        return getPointer(functype, {{getIndex(0), arrayDom}});
    // Zero initializer
    } else if (llvm::isa<llvm::ConstantAggregateZero>(constant)) {
        return getBottom(*constant->getType());
    // Undef
    } else if (llvm::isa<llvm::UndefValue>(constant)) {
        return getBottom(*constant->getType());
    // otherwise
    } else {
        errs() << "Unknown constant: " << ST_->toString(constant) << endl;
        return getTop(*constant->getType());
    }
}

Domain::Ptr DomainFactory::allocate(const llvm::Type& type) {
    // Void type - do nothing
    if (type.isVoidTy()) {
        return nullptr;
    // Simple type - allocating like array
    } else if (type.isIntegerTy() || type.isFloatingPointTy()) {
        auto&& arrayType = llvm::ArrayType::get(const_cast<llvm::Type*>(&type), 1);
        return allocate(*arrayType);
    // Struct or Array type
    } else if (type.isAggregateType()) {
        return getAggregate(type);
    // Function
    } else if (type.isFunctionTy()) {
        auto&& arrayType = llvm::ArrayType::get(const_cast<llvm::Type*>(&type), 1);
        return getAggregate(*arrayType, {getFunction(type)});
    // PointerDomain
    } else if (type.isPointerTy()) {
        auto&& location = allocate(*type.getPointerElementType());
        return getPointer(*type.getPointerElementType(), { {getIndex(0), location} });
    // Otherwise
    } else {
        errs() << "Creating domain of unknown type <" << ST_->toString(&type) << ">" << endl;
        return nullptr;
    }
}

Integer::Ptr DomainFactory::toInteger(uint64_t val, size_t width, bool isSigned) {
    return Integer::Ptr{ new IntValue(llvm::APInt(width, val, isSigned), width) };
}

Integer::Ptr DomainFactory::toInteger(const llvm::APInt& val) {
    return Integer::Ptr{ new IntValue(val, val.getBitWidth()) };
}

Domain::Ptr DomainFactory::getBool(bool value) {
    auto&& retval = value ? toInteger(1, 1) : toInteger(0, 1);
    return getInteger(retval);
}

Domain::Ptr DomainFactory::getIndex(uint64_t indx) {
    return getInteger(toInteger(indx, 64));
}

/* Integer */
Domain::Ptr DomainFactory::getInteger(size_t width) {
    return getInteger(Domain::BOTTOM, width);
}

Domain::Ptr DomainFactory::getInteger(Integer::Ptr val) {
    return getInteger(val, val);
}

Domain::Ptr DomainFactory::getInteger(Domain::Value value, size_t width) {
    return int_cache_[std::make_tuple(value,
                                      toInteger(0, width),
                                      toInteger(0, width),
                                      toInteger(0, width),
                                      toInteger(0, width))];
}

Domain::Ptr DomainFactory::getInteger(Integer::Ptr from, Integer::Ptr to) {
    return getInteger(from, to, from, to);
}

Domain::Ptr DomainFactory::getInteger(Integer::Ptr from, Integer::Ptr to, Integer::Ptr sfrom, Integer::Ptr sto) {
    return int_cache_[std::make_tuple(Domain::VALUE,
                                      from, to,
                                      sfrom, sto)];
}

/* Float */
Domain::Ptr DomainFactory::getFloat(const llvm::fltSemantics& semantics) {
    return getFloat(Domain::BOTTOM, semantics);
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& val) {
    return getFloat(val, val);
}

Domain::Ptr DomainFactory::getFloat(Domain::Value value, const llvm::fltSemantics& semantics) {
    return float_cache_[std::make_tuple(value,
                                        llvm::APFloat(semantics),
                                        llvm::APFloat(semantics))];
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& from, const llvm::APFloat& to) {
    return float_cache_[std::make_tuple(Domain::VALUE,
                                        llvm::APFloat(from),
                                        llvm::APFloat(to))];
}


/* PointerDomain */
Domain::Ptr DomainFactory::getNullptr(const llvm::Type& elementType) {
    static PointerLocation nullptrLocation{ getIndex(0), getNullptrLocation() };
    return Domain::Ptr{ new PointerDomain(this, elementType, {nullptrLocation}) };
}

Domain::Ptr DomainFactory::getNullptrLocation() {
    return nullptr_;
}

Domain::Ptr DomainFactory::getPointer(Domain::Value value, const llvm::Type& elementType) {
    return Domain::Ptr{ new PointerDomain(value, this, elementType) };
}

Domain::Ptr DomainFactory::getPointer(const llvm::Type& elementType) {
    return getPointer(Domain::BOTTOM, elementType);
}

Domain::Ptr DomainFactory::getPointer(const llvm::Type& elementType, const PointerDomain::Locations& locations) {
    return Domain::Ptr{ new PointerDomain(this, elementType, locations) };
}

/* AggregateDomain */
Domain::Ptr DomainFactory::getAggregate(const llvm::Type& type) {
    return getAggregate(Domain::VALUE, type);
}

Domain::Ptr DomainFactory::getAggregate(Domain::Value value, const llvm::Type& type) {
    if (type.isArrayTy()) {
        return Domain::Ptr{
                new AggregateDomain(value,
                                    this,
                                    *type.getArrayElementType(),
                                    getIndex(type.getArrayNumElements())
                )
        };

    } else if (type.isStructTy()) {
        AggregateDomain::Types types;
        for (auto i = 0U; i < type.getStructNumElements(); ++i)
            types[i] = type.getStructElementType(i);

        return Domain::Ptr{
                new AggregateDomain(value,
                                    this,
                                    types,
                                    getIndex(type.getStructNumElements())
                )
        };
    }
    UNREACHABLE("Unknown aggregate type: " + ST_->toString(&type));
}

Domain::Ptr DomainFactory::getAggregate(const llvm::Type& type, std::vector<Domain::Ptr> elements) {
    if (type.isArrayTy()) {
        AggregateDomain::Elements elementMap;
        for (auto i = 0U; i < elements.size(); ++i) {
            elementMap[i] = elements[i];
        }
        return Domain::Ptr { new AggregateDomain(this, *type.getArrayElementType(), elementMap) };

    } else if (type.isStructTy()) {
        AggregateDomain::Types types;
        AggregateDomain::Elements elementMap;
        for (auto i = 0U; i < type.getStructNumElements(); ++i) {
            types[i] = type.getStructElementType(i);
            elementMap[i] = elements[i];
        }
        return Domain::Ptr { new AggregateDomain(this, types, elementMap) };
    }
    UNREACHABLE("Unknown aggregate type: " + ST_->toString(&type));
}

/* Function */
Domain::Ptr DomainFactory::getFunction(const llvm::Type& type) {
    ASSERT(type.isFunctionTy(), "Trying to create function domain on non-function type");
    return Domain::Ptr{ new FunctionDomain(this, &type) };
}

Domain::Ptr DomainFactory::getFunction(const llvm::Type& type, Function::Ptr function) {
    ASSERT(type.isFunctionTy(), "Trying to create function domain on non-function type");
    return Domain::Ptr{ new FunctionDomain(this, &type, function) };
}

Domain::Ptr DomainFactory::getFunction(const llvm::Type& type, const FunctionDomain::FunctionSet& functions) {
    ASSERT(type.isFunctionTy(), "Trying to create function domain on non-function type");
    return Domain::Ptr{ new FunctionDomain(this, &type, functions) };
}

/* heap */
Domain::Ptr DomainFactory::getConstOperand(const llvm::Constant* c) {
    if (llvm::isa<llvm::GlobalVariable>(c)) return module_->findGlobal(c);
    else return get(c);
}


#define HANDLE_BINOP(OPER) \
    lhv = getConstOperand(ce->getOperand(0)); \
    rhv = getConstOperand(ce->getOperand(1)); \
    ASSERT(lhv && rhv, "Unknown binary constexpr operands"); \
    return lhv->OPER(rhv);

#define HANDLE_CAST(OPER) \
    lhv = getConstOperand(ce->getOperand(0)); \
    ASSERT(lhv, "Unknown cast constexpr operand"); \
    return lhv->OPER(*ce->getType());

Domain::Ptr DomainFactory::interpretConstantExpr(const llvm::ConstantExpr* ce) {
    Domain::Ptr lhv, rhv;
    switch (ce->getOpcode()) {
        case llvm::Instruction::Add: HANDLE_BINOP(add);
        case llvm::Instruction::FAdd: HANDLE_BINOP(fadd);
        case llvm::Instruction::Sub: HANDLE_BINOP(sub);
        case llvm::Instruction::FSub: HANDLE_BINOP(fsub);
        case llvm::Instruction::Mul: HANDLE_BINOP(mul);
        case llvm::Instruction::FMul: HANDLE_BINOP(fmul);
        case llvm::Instruction::UDiv: HANDLE_BINOP(udiv);
        case llvm::Instruction::SDiv: HANDLE_BINOP(sdiv);
        case llvm::Instruction::FDiv: HANDLE_BINOP(fdiv);
        case llvm::Instruction::URem: HANDLE_BINOP(urem);
        case llvm::Instruction::SRem: HANDLE_BINOP(srem);
        case llvm::Instruction::FRem: HANDLE_BINOP(frem);
        case llvm::Instruction::And: HANDLE_BINOP(bAnd);
        case llvm::Instruction::Or: HANDLE_BINOP(bOr);
        case llvm::Instruction::Xor: HANDLE_BINOP(bXor);
        case llvm::Instruction::Shl: HANDLE_BINOP(shl);
        case llvm::Instruction::LShr: HANDLE_BINOP(lshr);
        case llvm::Instruction::AShr: HANDLE_BINOP(ashr);
        case llvm::Instruction::Trunc: HANDLE_CAST(trunc);
        case llvm::Instruction::SExt: HANDLE_CAST(sext);
        case llvm::Instruction::ZExt: HANDLE_CAST(zext);
        case llvm::Instruction::ICmp:
            lhv = getConstOperand(ce->getOperand(0));
            rhv = getConstOperand(ce->getOperand(1));
            return lhv->icmp(rhv, static_cast<llvm::ICmpInst::Predicate>(ce->getPredicate()));
        case llvm::Instruction::FCmp:
            lhv = getConstOperand(ce->getOperand(0));
            rhv = getConstOperand(ce->getOperand(1));
            return lhv->fcmp(rhv, static_cast<llvm::FCmpInst::Predicate>(ce->getPredicate()));
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
            UNREACHABLE("Unknown opcode of ConstExpr: " + ST_->toString(ce));
    }
}

Domain::Ptr DomainFactory::handleGEPConstantExpr(const llvm::ConstantExpr* ce) {
    auto&& ptr = getConstOperand(ce->getOperand(0));
    ASSERT(ptr, "gep args: " + ST_->toString(ce->getOperand(0)));

    std::vector<Domain::Ptr> offsets;
    for (auto j = 1U; j != ce->getNumOperands(); ++j) {
        auto val = ce->getOperand(j);
        if (auto&& intConstant = llvm::dyn_cast<llvm::ConstantInt>(val)) {
            offsets.emplace_back(getIndex(*intConstant->getValue().getRawData()));
        } else if (auto indx = getConstOperand(val)) {
            offsets.emplace_back(indx);
        } else {
            UNREACHABLE("Non-integer constant in gep");
        }
    }
    return ptr->gep(*ce->getType()->getPointerElementType(), offsets);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"