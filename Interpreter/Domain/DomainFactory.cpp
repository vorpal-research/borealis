//
// Created by abdullin on 2/7/17.
//

#include <llvm/IR/DerivedTypes.h>
#include <Type/TypeUtils.h>

#include "DomainFactory.h"

#include "Numerical/Interval.hpp"

#include "Interpreter/Domain/AggregateDomain.h"
#include "Interpreter/Domain/Integer/IntValue.h"
#include "Interpreter/IR/Module.h"
#include "Interpreter/Util.hpp"
#include "Util/cast.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {


AbstractDomain::Ptr AbstractFactory::top(Type::Ptr type) const {
    return nullptr;
//    if (llvm::isa<type::UnknownType>(type.get())) {
//        return nullptr;
//    } else if (llvm::isa<type::Bool>(type.get())) {
//        return getBool(TOP);
//    } else if (llvm::isa<type::Integer>(type.get())) {
//        return getInt(type, TOP);
//    } else if (llvm::isa<type::Float>(type.get())) {
//        return getFloat(type, TOP);
//    } else if (llvm::isa<type::Array>(type.get())) {
//        return getArray(type, TOP);
//    } else if (llvm::isa<type::Record>(type.get())) {
//        return getStruct(type, TOP);
//    } else if (llvm::isa<type::Function>(type.get())) {
//        // TODO
//        return nullptr;
//    } else if (llvm::isa<type::Pointer>(type.get())) {
//        return getPointer(type, TOP);
//    } else {
//        errs() << "Creating domain of unknown type <" << type << ">" << endl;
//        return nullptr;
//    }
}

AbstractDomain::Ptr AbstractFactory::bottom(Type::Ptr type) const {
    return nullptr;
//    if (llvm::isa<type::UnknownType>(type.get())) {
//        return nullptr;
//    } else if (llvm::isa<type::Bool>(type.get())) {
//        return getBool(BOTTOM);
//    } else if (llvm::isa<type::Integer>(type.get())) {
//        return getInt(type, BOTTOM);
//    } else if (llvm::isa<type::Float>(type.get())) {
//        return getFloat(type, BOTTOM);
//    } else if (llvm::isa<type::Array>(type.get())) {
//        return getArray(type, BOTTOM);
//    } else if (llvm::isa<type::Record>(type.get())) {
//        return getStruct(type, BOTTOM);
//    } else if (llvm::isa<type::Function>(type.get())) {
//        // TODO
//        return nullptr;
//    } else if (llvm::isa<type::Pointer>(type.get())) {
//        return getPointer(type, BOTTOM);
//    } else {
//        errs() << "Creating domain of unknown type <" << type << ">" << endl;
//        return nullptr;
//    }
}


AbstractDomain::Ptr AbstractFactory::getBool(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return BoolT::top();
    } else if (kind == BOTTOM) {
        return BoolT::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getBool(bool value) const {
    return BoolT::constant((int) value);
}

AbstractDomain::Ptr AbstractFactory::getInteger(Type::Ptr type, AbstractFactory::Kind kind, bool sign) const {
    auto* integer = llvm::dyn_cast<type::Integer>(type.get());
    if (integer->getBitsize() == 32) return getInt(kind, sign);
    else if (integer->getBitsize() == 64) return getLong(kind, sign);
    else {
        if (kind == TOP) {
            if (sign) {
                return Interval<BitInt<true>>::top();
            } else {
                return Interval<BitInt<false>>::top();
            }
        } else if (kind == BOTTOM) {
            if (sign) {
                return Interval<BitInt<true>>::bottom();
            } else {
                return Interval<BitInt<false>>::bottom();
            }
        } else {
            UNREACHABLE("Unknown kind");
        }
    }
}

AbstractDomain::Ptr AbstractFactory::getInteger(unsigned long long n, unsigned width, bool sign) const {
    if (width == 32) return getInt(n, sign);
    else if (width == 64) return getLong(n, sign);
    else {
        if (sign) {
            return Interval<BitInt<true>>::constant(BitInt<true>(width, n));
        } else {
            return Interval<BitInt<false>>::constant(BitInt<false>(width, n));
        }
    }
}

AbstractDomain::Ptr AbstractFactory::getInt(AbstractFactory::Kind kind, bool sign) const {
    if (kind == TOP) {
        if (sign) {
            return Interval<Int<32U, true>>::top();
        } else {
            return Interval<Int<32U, false>>::top();
        }
    } else if (kind == BOTTOM) {
        if (sign) {
            return Interval<Int<32U, true>>::bottom();
        } else {
            return Interval<Int<32U, false>>::bottom();
        }
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getInt(int n, bool sign) const {
    if (sign) {
        return Interval<Int<32U, true>>::constant(n);
    } else {
        return Interval<Int<32U, false>>::constant(n);
    }
}

AbstractDomain::Ptr AbstractFactory::getLong(AbstractFactory::Kind kind, bool sign) const {
    if (kind == TOP) {
        if (sign) {
            return Interval<Int<64U, true>>::top();
        } else {
            return Interval<Int<64U, false>>::top();
        }
    } else if (kind == BOTTOM) {
        if (sign) {
            return Interval<Int<64U, true>>::bottom();
        } else {
            return Interval<Int<64U, false>>::bottom();
        }
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getLong(long n, bool sign) const {
    if (sign) {
        return Interval<Int<64U, true>>::constant(n);
    } else {
        return Interval<Int<64U, false>>::constant(n);
    }
}

AbstractDomain::Ptr AbstractFactory::getFloat(AbstractFactory::Kind kind) const {
    if (kind == TOP) {
        return FloatT::top();
    } else if (kind == BOTTOM) {
        return FloatT::bottom();
    } else {
        UNREACHABLE("Unknown kind");
    }
}

AbstractDomain::Ptr AbstractFactory::getFloat(double n) const {
    return FloatT::constant(n);
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DomainFactory::DomainFactory(SlotTrackerPass* ST, ir::GlobalVariableManager* GM, const llvm::DataLayout* DL)
        : ObjectLevelLogging("domain"),
          ST_(ST),
          GVM_(GM),
          DL_(DL),
          TF_(TypeFactory::get()),
          int_cache_(LAM(a, std::make_shared<IntegerIntervalDomain>(this, a))),
          float_cache_(LAM(a, std::make_shared<FloatIntervalDomain>(this, a))),
          nullptr_{ std::make_shared<NullptrDomain>(this) },
          nullptrLocation_{ {getIndex(0)}, nullptr_ } {}

Type::Ptr DomainFactory::cast(const llvm::Type* type) {
    auto res = TF_->cast(type, DL_);
    if (llvm::isa<type::Bool>(res)) return TF_->getInteger(1);
    else return res;
}

#define GENERATE_GET_VALUE(VALUE) \
    if (llvm::isa<type::UnknownType>(type.get())) { \
        return nullptr; \
    } else if (llvm::isa<type::Bool>(type.get())) { \
        return getInteger(VALUE, TF_->getInteger(1)); \
    } else if (llvm::isa<type::Integer>(type.get())) { \
        return getInteger(VALUE, type); \
    } else if (llvm::isa<type::Float>(type.get())) { \
        return getFloat(VALUE); \
    } else if (llvm::isa<type::Array>(type.get())) { \
        return getAggregate(VALUE, type); \
    } else if (llvm::isa<type::Record>(type.get())) { \
        return getAggregate(VALUE, type); \
    } else if (llvm::isa<type::Function>(type.get())) { \
        return getFunction(type); \
    } else if (auto ptr = llvm::dyn_cast<type::Pointer>(type.get())) { \
        return getPointer(VALUE, ptr->getPointed()); \
    } else { \
        errs() << "Creating domain of unknown type <" << type << ">" << endl; \
        return nullptr; \
    }


/*  General */
Domain::Ptr DomainFactory::getTop(Type::Ptr type) {
    GENERATE_GET_VALUE(Domain::TOP);
}

Domain::Ptr DomainFactory::getBottom(Type::Ptr type) {
    GENERATE_GET_VALUE(Domain::BOTTOM);
}

Domain::Ptr DomainFactory::get(const llvm::Value* val) {
    auto&& type = cast(val->getType());
    if (llvm::isa<type::UnknownType>(type.get())) {
        return nullptr;
    } else if (llvm::isa<type::Bool>(type.get())) {
        return getInteger(TF_->getInteger(1));
    } else if (llvm::isa<type::Integer>(type.get())) {
        return getInteger(type);
    } else if (llvm::isa<type::Float>(type.get())) {
        return getFloat();
    } else if (llvm::isa<type::Array>(type.get())) {
        return getAggregate(type);
    } else if (llvm::isa<type::Record>(type.get())) {
        return getAggregate(type);
    } else if (llvm::isa<type::Function>(type.get())) {
        return getFunction(type);
    } else if (llvm::isa<type::Pointer>(type.get())) {
        return allocate(type);
    } else {
        errs() << "Creating domain of unknown type <" << type << ">" << endl;
        return nullptr;
    }
}

Domain::Ptr DomainFactory::get(const llvm::Constant* constant) {
    // Integer
    if (auto intConstant = llvm::dyn_cast<llvm::ConstantInt>(constant)) {
        return getInteger(toInteger(intConstant->getValue()));
    // Float
    } else if (auto floatConstant = llvm::dyn_cast<llvm::ConstantFP>(constant)) {
        return getFloat(floatConstant->getValueAPF());
    // Null pointer
    } else if (llvm::isa<llvm::ConstantPointerNull>(constant)) {
        return getNullptr(cast(constant->getType()->getPointerElementType()));
    // Constant Data Array
    } else if (auto sequential = llvm::dyn_cast<llvm::ConstantDataSequential>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < sequential->getNumElements(); ++i) {
            auto element = get(sequential->getElementAsConstant(i));
            if (not element) {
                errs() << "Cannot create constant: " << ST_->toString(sequential->getElementAsConstant(i)) << endl;
                element = getTop(cast(sequential->getType()));
            }
            elements.emplace_back(element);
        }
        return getAggregate(cast(constant->getType()), elements);
    // Constant Array
    } else if (auto constantArray = llvm::dyn_cast<llvm::ConstantArray>(constant)) {
        std::vector<Domain::Ptr> elements;
        for (auto i = 0U; i < constantArray->getNumOperands(); ++i) {
            auto element = get(constantArray->getOperand(i));
            if (not element) {
                errs() << "Cannot create constant: " << ST_->toString(constantArray->getOperand(i)) << endl;
                element = getTop(cast(sequential->getType()));
            }
            elements.emplace_back(element);
        }
        return getAggregate(cast(constant->getType()), elements);
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
                element = getTop(cast(sequential->getType()));
            }
            elements.emplace_back(element);
        }
        return getAggregate(cast(constant->getType()), elements);
    // Function
    } else if (auto function = llvm::dyn_cast<llvm::Function>(constant)) {
        auto&& functype = cast(function->getType()->getPointerElementType());
        auto&& arrayType = TF_->getArray(cast(function->getType()), 1);
        auto&& arrayDom = getAggregate(arrayType, {getFunction(functype, GVM_->get(function))});
        return getPointer(functype, {{{getIndex(0)}, arrayDom}});
    // Zero initializer
    } else if (llvm::isa<llvm::ConstantAggregateZero>(constant)) {
       return getTop(cast(constant->getType()));
    // Undef
    } else if (llvm::isa<llvm::UndefValue>(constant)) {
        return getBottom(cast(constant->getType()));
    // otherwise
    } else {
        errs() << "Unknown constant: " << ST_->toString(constant) << endl;
        return getTop(cast(constant->getType()));
    }
}

Domain::Ptr DomainFactory::get(const llvm::GlobalVariable* global) {
    Domain::Ptr globalDomain;
    if (global->hasInitializer()) {
        Domain::Ptr content;
        auto& elementType = *global->getType()->getPointerElementType();
        // If global is simple type, that we should wrap it like array of size 1
        if (elementType.isIntegerTy() || elementType.isFloatingPointTy()) {
            auto simpleConst = get(global->getInitializer());
			if (simpleConst->isTop()) return getTop(cast(global->getType()));
            auto&& arrayType = TF_->getArray(cast(&elementType), 1);
            content = getAggregate(arrayType, {simpleConst});
            // else just create domain
        } else {
            content = get(global->getInitializer());
			if (content->isTop()) return getTop(cast(global->getType()));
        }
        ASSERT(content, "Unsupported constant");

        PointerLocation loc = {{getIndex(0)}, content};
        auto newDomain = getPointer(cast(&elementType), {loc});
        // we need this because GEPs for global structs and arrays contain one additional index at the start
        if (not (elementType.isIntegerTy() || elementType.isFloatingPointTy())) {
            auto newArray = TF_->getArray(cast(global->getType()), 1);
            auto newLevel = getAggregate(newArray, {newDomain});
            PointerLocation loc2 = {{getIndex(0)}, newLevel};
            globalDomain = getPointer(newArray, {loc2});
        } else {
            globalDomain = newDomain;
        }

    } else {
        globalDomain = getBottom(cast(global->getType()));
    }

    ASSERT(globalDomain, "Could not create domain for: " + ST_->toString(global));
    return globalDomain;
}

Domain::Ptr DomainFactory::allocate(Type::Ptr type) {
    if (llvm::isa<type::UnknownType>(type.get())) {
        return nullptr;
    } else if (llvm::isa<type::Bool>(type.get())) {
        auto intTy = TF_->getInteger(1);
        auto arrayTy = TF_->getArray(intTy, 1);
        return allocate(arrayTy);
    } else if (llvm::is_one_of<type::Integer, type::Float>(type.get())) {
        auto arrayTy = TF_->getArray(type, 1);
        return allocate(arrayTy);
    } else if (llvm::isa<type::Array>(type.get())) {
        return getAggregate(type);
    } else if (llvm::isa<type::Record>(type.get())) {
        return getAggregate(type);
    } else if (llvm::isa<type::Function>(type.get())) {
        auto arrayTy = TF_->getArray(type, 1);
        return getAggregate(arrayTy, {getFunction(type)});
    } else if (auto ptr = llvm::dyn_cast<type::Pointer>(type.get())) {
        auto location = allocate(ptr->getPointed());
        return getPointer(ptr->getPointed(), { {{getIndex(0)}, location} });
    } else {
        errs() << "Creating domain of unknown type <" << type << ">" << endl;
        return nullptr;
    }
}

Integer::Ptr DomainFactory::toInteger(uint64_t val, size_t width, bool isSigned) {
    return toInteger(llvm::APInt(width, val, isSigned));
}

Integer::Ptr DomainFactory::toInteger(const llvm::APInt& val) {
    return Integer::getValue(val);
}

Domain::Ptr DomainFactory::getBool(bool value) {
    auto&& retval = value ? toInteger(1, 1) : toInteger(0, 1);
    return getInteger(retval);
}

Domain::Ptr DomainFactory::getIndex(uint64_t indx) {
    return getInteger(toInteger(indx, DomainFactory::defaultSize));
}

/* Integer */
Domain::Ptr DomainFactory::getInteger(Type::Ptr type) {
    return getInteger(Domain::BOTTOM, type);
}

Domain::Ptr DomainFactory::getInteger(Integer::Ptr val) {
    return getInteger(val, val);
}

Domain::Ptr DomainFactory::getInteger(Domain::Value value, Type::Ptr type) {
    auto intType = llvm::dyn_cast<type::Integer>(type.get());
    ASSERT(intType, "Non-integer type in getInteger()");
    auto width = intType->getBitsize();
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
Domain::Ptr DomainFactory::getFloat() {
    return getFloat(Domain::BOTTOM);
}

Domain::Ptr DomainFactory::getFloat(double val) {
    return getFloat(llvm::APFloat(val));
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& val) {
    return getFloat(val, val);
}

Domain::Ptr DomainFactory::getFloat(Domain::Value value) {
    auto& semantics = FloatIntervalDomain::getSemantics();
    return float_cache_[std::make_tuple(value,
                                        llvm::APFloat(semantics),
                                        llvm::APFloat(semantics))];
}

Domain::Ptr DomainFactory::getFloat(const llvm::APFloat& from, const llvm::APFloat& to) {
    auto& semantics = FloatIntervalDomain::getSemantics();
    auto roundingMode = FloatIntervalDomain::getRoundingMode();
    llvm::APFloat lb(from), ub(to);
    bool isSuccessfull = true;
    if (lb.convert(semantics, roundingMode, &isSuccessfull) != llvm::APFloat::opOK) {
        lb = util::getMinValue(semantics);
    }
    if (ub.convert(semantics, roundingMode, &isSuccessfull) != llvm::APFloat::opOK) {
        ub = util::getMaxValue(semantics);
    }
    ub.convert(semantics, roundingMode, &isSuccessfull);
    return float_cache_[std::make_tuple(Domain::VALUE,
                                        lb,
                                        ub)];
}


/* PointerDomain */
Domain::Ptr DomainFactory::getNullptr(Type::Ptr elementType) {
    return std::make_shared<PointerDomain>(this, elementType, PointerDomain::Locations{nullptrLocation_});
}

Domain::Ptr DomainFactory::getNullptrLocation() {
    return nullptr_;
}

Domain::Ptr DomainFactory::getPointer(Domain::Value value, Type::Ptr elementType, bool isGep) {
    return std::make_shared<PointerDomain>(value, this, elementType, isGep);
}

Domain::Ptr DomainFactory::getPointer(Type::Ptr elementType, bool isGep) {
    return getPointer(Domain::BOTTOM, elementType, isGep);
}

Domain::Ptr DomainFactory::getPointer(Type::Ptr elementType, const PointerDomain::Locations& locations, bool isGep) {
    return std::make_shared<PointerDomain>(this, elementType, locations, isGep);
}

/* AggregateDomain */
Domain::Ptr DomainFactory::getAggregate(Type::Ptr type) {
    return getAggregate(Domain::VALUE, type);
}

Domain::Ptr DomainFactory::getAggregate(Domain::Value value, Type::Ptr type) {
    // we can't create BOTTOM aggregate, just aggregate with BOTTOM elements
    if (value == Domain::BOTTOM) value = Domain::VALUE;
    if (auto array = llvm::dyn_cast<type::Array>(type.get())) {
        auto size = array->getSize() ?
                      array->getSize().getUnsafe() :
                      0;
        return std::make_shared<AggregateDomain>(value, this,
                                                 array->getElement(),
                                                 getIndex(size));

    } else if (auto record = llvm::dyn_cast<type::Record>(type.get())) {
        auto& body = record->getBody()->get();
        AggregateDomain::Types types;
        for (auto i = 0U; i < body.getNumFields(); ++i)
            types.emplace_back(body.at(i).getType());

        return std::make_shared<AggregateDomain>(value, this, types, getIndex(body.getNumFields()));
    }
    UNREACHABLE("Unknown aggregate type: " + TypeUtils::toString(*type.get()));
}

Domain::Ptr DomainFactory::getAggregate(Type::Ptr type, std::vector<Domain::Ptr> elements) {
    if (auto array = llvm::dyn_cast<type::Array>(type.get())) {
        AggregateDomain::Elements elementMap;
        for (auto i = 0U; i < elements.size(); ++i) {
            elementMap[i] = elements[i];
        }
        return std::make_shared<AggregateDomain>(this, array->getElement(), elementMap);

    } else if (auto record = llvm::dyn_cast<type::Record>(type.get())) {
        auto& body = record->getBody()->get();
        AggregateDomain::Types types;
        AggregateDomain::Elements elementMap;
        for (auto i = 0U; i < body.getNumFields(); ++i) {
            types.emplace_back(body.at(i).getType());
            elementMap[i] = elements[i];
        }
        return std::make_shared<AggregateDomain>(this, types, elementMap);
    }
    UNREACHABLE("Unknown aggregate type: " + TypeUtils::toString(*type.get()));
}

/* Function */
Domain::Ptr DomainFactory::getFunction(Type::Ptr type) {
    ASSERT(llvm::isa<type::Function>(type.get()), "Trying to create function domain on non-function type");
    return std::make_shared<FunctionDomain>(this, type);
}

Domain::Ptr DomainFactory::getFunction(Type::Ptr type, ir::Function::Ptr function) {
    ASSERT(llvm::isa<type::Function>(type.get()), "Trying to create function domain on non-function type");
    return std::make_shared<FunctionDomain>(this, type, function);
}

Domain::Ptr DomainFactory::getFunction(Type::Ptr type, const FunctionDomain::FunctionSet& functions) {
    ASSERT(llvm::isa<type::Function>(type.get()), "Trying to create function domain on non-function type");
    return std::make_shared<FunctionDomain>(this, type, functions);
}

/* heap */
Domain::Ptr DomainFactory::getConstOperand(const llvm::Constant* c) {
    if (llvm::isa<llvm::GlobalVariable>(c)) return GVM_->findGlobal(c);
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
    return lhv->OPER(cast(ce->getType()));

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
    return ptr->gep(cast(ce->getType()->getPointerElementType()), offsets);
}

}   /* namespace absint */
}   /* namespace borealis */

#include "Util/unmacros.h"
