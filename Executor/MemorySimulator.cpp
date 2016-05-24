/*
 * MemorySimulator.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: belyaev
 */

#include <llvm/Support/Host.h>

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <unordered_map>
#include <map>

#include <tinyformat/tinyformat.h>
#include <llvm/ExecutionEngine/GenericValue.h>

#include "Executor/MemorySimulator.h"
#include "Executor/Exceptions.h"
#include "Util/util.h"
#include "Util/collections.hpp"
#include "Config/config.h"

#include "Executor/MemorySimulator/SegmentTreeImpl.h"
#include "Executor/MemorySimulator/GlobalMemory.h"

#include "Logging/tracer.hpp"

#include "Util/macros.h"

namespace borealis {

template<class T>
static llvm::ArrayRef<uint8_t> bufferOfPod(const T& value, size_t bytes) {
    TRACE_FUNC;
    return llvm::ArrayRef<uint8_t>{ static_cast<const uint8_t*>(static_cast<const void*>(&value)), bytes };
}

static SimulatedPtrSize ptrSub(const void* p1, const void* p2) {
    return static_cast<SimulatedPtrSize>( static_cast<const uint8_t*>(p1) - static_cast<const uint8_t*>(p2) );
}

namespace {
struct hex {
    const llvm::ArrayRef<uint8_t>* buf;
    hex(const llvm::ArrayRef<uint8_t>& buf): buf(&buf) {}

    friend std::ostream& operator<<(std::ostream& ost, const hex& h) {
        for(auto&& byte: *h.buf) ost << std::hex << std::setw(2) << std::setfill('0') << +byte;
        return ost;
    }
};
}


struct MemorySimulator::Impl {
    SegmentTree tree;
    std::unordered_map<llvm::Value*, SimulatedPtr> constants;
    std::map<SimulatedPtr, llvm::Value*> constantsBwd;

    SimulatedPtr unmeaningfulPtr;

    SimulatedPtr allocStart;
    SimulatedPtr allocEnd;

    SimulatedPtr mallocStart;
    SimulatedPtr mallocEnd;

    SimulatedPtr constantStart;
    SimulatedPtr constantEnd;

    SimulatedPtrSize currentAllocOffset;
    SimulatedPtrSize currentMallocOffset;
    SimulatedPtrSize currentConstantOffset;

    globalMemoryTable globals;
    const llvm::DataLayout* DL;

    Impl(const llvm::DataLayout& dl): DL(&dl) {
        TRACE_FUNC;

        auto grain = dl.getPointerPrefAlignment(0);
        if(!grain) grain = dl.getPointerSize(0);
        if(!grain) grain = 1;
        tree.chunk_size = K.get(1) * grain;
        tree.start = 1 << 20;
        tree.end = tree.start + (1ULL << M.get(33ULL)) * tree.chunk_size;

        allocStart = tree.start;
        allocEnd = (tree.end - tree.start) / 2 + tree.start;

        mallocStart = allocEnd;
        mallocEnd = tree.end;

        constantStart = 1 << 10;
        constantEnd = 1 << 20;

        unmeaningfulPtr = 1 << 8;

        currentAllocOffset = 0U;
        currentMallocOffset = 0U;
        currentConstantOffset = 0U;
    }

    void* getPointerToConstant(llvm::Value* v, size_t size) {
        TRACE_FUNC;
        if(constants.count(v)) return ptr_cast(constants.at(v));
        else {
            auto realPtr = currentConstantOffset + constantStart;
            currentConstantOffset += size;
            constants[v] = realPtr;
            constantsBwd[realPtr] = v;
            return ptr_cast(realPtr);
        }
    }

    void* getPointerToGlobalMemory(MemorySimulator* parent,
            llvm::GlobalValue* gv, SimulatedPtrSize size, SimulatedPtrSize offset) {
        TRACE_FUNC;
        // FIXME: check offset for validity and throw an exception

        if(DL->getTypeStoreSize(gv->getType()->getPointerElementType()) < (offset+size)) {
            return nullptr;
        }

        if (auto F = const_cast<llvm::Function*>(llvm::dyn_cast<llvm::Function>(gv))) {
            return getPointerToConstant(F, size);
        }

        if (auto&& pp = util::at(globals, llvm::dyn_cast<llvm::GlobalVariable>(gv)))
          return pp.getUnsafe().get() + offset;

        // Global variable might have been added since interpreter started.
        if (llvm::GlobalVariable *GVar =
                const_cast<llvm::GlobalVariable *>(llvm::dyn_cast<llvm::GlobalVariable>(gv))) {
            globals[GVar] = allocateMemoryForGV(GVar, *DL);
            TRACE_PARAM(hex(buffer_t{globals[GVar].get(), size}));

            if(GVar->hasInitializer()) parent->initializeMemory(GVar->getInitializer(), globals[GVar].get());

            TRACE_PARAM(hex(buffer_t{globals[GVar].get(), size}));

            TRACE_PARAM(globals[GVar].get() + offset);
            return globals[GVar].get() + offset;
        } else UNREACHABLE("Global hasn't had an address allocated yet!");

        return nullptr;
    }

    bool isFromGlobalSpace(SimulatedPtr ptr) const {
        return constantStart <= ptr && constantEnd > ptr;
    }

    bool isFromDynMem(SimulatedPtr ptr) const {
        return mallocStart <= ptr && mallocEnd > ptr;
    }

    bool isFromStack(SimulatedPtr ptr) const {
        return allocStart <= ptr && allocEnd > ptr;
    }


    bool isAPointer(SimulatedPtr ptr) const {
        return isFromGlobalSpace(ptr)
            || isFromDynMem(ptr)
            || isFromStack(ptr)
            || ptr == unmeaningfulPtr;
    }

    ~Impl(){}
};

uintptr_t MemorySimulator::getQuant() const {
    return pimpl_->tree.chunk_size;
}

void* MemorySimulator::getOpaquePtr() {
    TRACE_FUNC;
    return ptr_cast(pimpl_->unmeaningfulPtr);
}

bool MemorySimulator::isOpaquePointer(const void* ptr) {
    TRACE_FUNC;
    return pimpl_->unmeaningfulPtr == ptr_cast(ptr);
}

bool MemorySimulator::isGlobalPointer(const void *ptr) {
    TRACE_FUNC;
    return pimpl_->isFromGlobalSpace(ptr_cast(ptr));
}

bool MemorySimulator::isLocalPointer(const void *ptr) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(ptr);
    return pimpl_->isFromDynMem(realPtr) || pimpl_->isFromStack(realPtr);
}

void* MemorySimulator::getPointerToFunction(llvm::Function* f, size_t size) {
    TRACE_FUNC;
    return pimpl_->getPointerToConstant(f, size);
}
void* MemorySimulator::getPointerBasicBlock(llvm::BasicBlock* bb, size_t size) {
    TRACE_FUNC;
    return pimpl_->getPointerToConstant(bb, size);
}
void* MemorySimulator::getPointerToGlobal(llvm::GlobalValue* gv, size_t size, SimulatedPtrSize offset) {
    TRACE_FUNC;

    return static_cast<uint8_t*>(pimpl_->getPointerToConstant(gv, size)) + offset;
}

llvm::GenericValue MemorySimulator::getConstantValue(const llvm::Constant *C) {
    TRACE_FUNC;
    using namespace llvm;
    auto DL = pimpl_->DL;
    // If its undefined, return the garbage.
    if (isa<UndefValue>(C)) {
        GenericValue Result;
        switch (C->getType()->getTypeID()) {
        default:
            break;
        case llvm::Type::IntegerTyID:
        case llvm::Type::X86_FP80TyID:
        case llvm::Type::FP128TyID:
        case llvm::Type::PPC_FP128TyID:
            // Although the value is undefined, we still have to construct an APInt
            // with the correct bit width.
            Result.IntVal = APInt(C->getType()->getPrimitiveSizeInBits(), 0);
            break;
        case llvm::Type::StructTyID: {
            // if the whole struct is 'undef' just reserve memory for the value.
            if(StructType *STy = dyn_cast<StructType>(C->getType())) {
                unsigned int elemNum = STy->getNumElements();
                Result.AggregateVal.resize(elemNum);
                for (unsigned int i = 0; i < elemNum; ++i) {
                    llvm::Type *ElemTy = STy->getElementType(i);
                    if (ElemTy->isIntegerTy())
                        Result.AggregateVal[i].IntVal =
                            APInt(ElemTy->getPrimitiveSizeInBits(), 0);
                    else if (ElemTy->isAggregateType()) {
                        const Constant *ElemUndef = UndefValue::get(ElemTy);
                        Result.AggregateVal[i] = getConstantValue(ElemUndef);
                    }
                }
            }
        }
        break;
        case llvm::Type::VectorTyID:
            // if the whole vector is 'undef' just reserve memory for the value.
            const VectorType* VTy = dyn_cast<VectorType>(C->getType());
            const llvm::Type *ElemTy = VTy->getElementType();
            unsigned int elemNum = VTy->getNumElements();
            Result.AggregateVal.resize(elemNum);
            if (ElemTy->isIntegerTy())
                for (unsigned int i = 0; i < elemNum; ++i)
                    Result.AggregateVal[i].IntVal =
                        APInt(ElemTy->getPrimitiveSizeInBits(), 0);
            break;
        }
        return Result;
    }

    // Otherwise, if the value is a ConstantExpr...
    if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
        Constant *Op0 = CE->getOperand(0);
        switch (CE->getOpcode()) {
        case Instruction::GetElementPtr: {
            // Compute the index
            GenericValue Result = getConstantValue(Op0);
            APInt Offset(DL->getPointerSizeInBits(), 0);
            cast<GEPOperator>(CE)->accumulateConstantOffset(*DL, Offset);

            char* tmp = (char*) Result.PointerVal;
            Result = PTOGV(tmp + Offset.getSExtValue());
            return Result;
        }
        case Instruction::Trunc: {
            GenericValue GV = getConstantValue(Op0);
            uint32_t BitWidth = cast<IntegerType>(CE->getType())->getBitWidth();
            GV.IntVal = GV.IntVal.trunc(BitWidth);
            return GV;
        }
        case Instruction::ZExt: {
            GenericValue GV = getConstantValue(Op0);
            uint32_t BitWidth = cast<IntegerType>(CE->getType())->getBitWidth();
            GV.IntVal = GV.IntVal.zext(BitWidth);
            return GV;
        }
        case Instruction::SExt: {
            GenericValue GV = getConstantValue(Op0);
            uint32_t BitWidth = cast<IntegerType>(CE->getType())->getBitWidth();
            GV.IntVal = GV.IntVal.sext(BitWidth);
            return GV;
        }
        case Instruction::FPTrunc: {
            // FIXME long double
            GenericValue GV = getConstantValue(Op0);
            GV.FloatVal = float(GV.DoubleVal);
            return GV;
        }
        case Instruction::FPExt:{
            // FIXME long double
            GenericValue GV = getConstantValue(Op0);
            GV.DoubleVal = double(GV.FloatVal);
            return GV;
        }
        case Instruction::UIToFP: {
            GenericValue GV = getConstantValue(Op0);
            if (CE->getType()->isFloatTy())
                GV.FloatVal = float(GV.IntVal.roundToDouble());
            else if (CE->getType()->isDoubleTy())
                GV.DoubleVal = GV.IntVal.roundToDouble();
            else if (CE->getType()->isX86_FP80Ty()) {
                APFloat apf = APFloat::getZero(APFloat::x87DoubleExtended);
                (void)apf.convertFromAPInt(GV.IntVal,
                    false,
                    APFloat::rmNearestTiesToEven);
                GV.IntVal = apf.bitcastToAPInt();
            }
            return GV;
        }
        case Instruction::SIToFP: {
            GenericValue GV = getConstantValue(Op0);
            if (CE->getType()->isFloatTy())
                GV.FloatVal = float(GV.IntVal.signedRoundToDouble());
            else if (CE->getType()->isDoubleTy())
                GV.DoubleVal = GV.IntVal.signedRoundToDouble();
            else if (CE->getType()->isX86_FP80Ty()) {
                APFloat apf = APFloat::getZero(APFloat::x87DoubleExtended);
                (void)apf.convertFromAPInt(GV.IntVal,
                    true,
                    APFloat::rmNearestTiesToEven);
                GV.IntVal = apf.bitcastToAPInt();
            }
            return GV;
        }
        case Instruction::FPToUI: // double->APInt conversion handles sign
        case Instruction::FPToSI: {
            GenericValue GV = getConstantValue(Op0);
            uint32_t BitWidth = cast<IntegerType>(CE->getType())->getBitWidth();
            if (Op0->getType()->isFloatTy())
                GV.IntVal = APIntOps::RoundFloatToAPInt(GV.FloatVal, BitWidth);
            else if (Op0->getType()->isDoubleTy())
                GV.IntVal = APIntOps::RoundDoubleToAPInt(GV.DoubleVal, BitWidth);
            else if (Op0->getType()->isX86_FP80Ty()) {
                APFloat apf = APFloat(APFloat::x87DoubleExtended, GV.IntVal);
                uint64_t v;
                bool ignored;
                (void)apf.convertToInteger(&v, BitWidth,
                    CE->getOpcode()==Instruction::FPToSI,
                    APFloat::rmTowardZero, &ignored);
                GV.IntVal = v; // endian?
            }
            return GV;
        }
        case Instruction::PtrToInt: {
            GenericValue GV = getConstantValue(Op0);
            uint32_t PtrWidth = DL->getTypeSizeInBits(Op0->getType());
            ASSERT(PtrWidth <= 64, "Bad pointer width");
            GV.IntVal = APInt(PtrWidth, uintptr_t(GV.PointerVal));
            uint32_t IntWidth = DL->getTypeSizeInBits(CE->getType());
            GV.IntVal = GV.IntVal.zextOrTrunc(IntWidth);
            return GV;
        }
        case Instruction::IntToPtr: {
            GenericValue GV = getConstantValue(Op0);
            uint32_t PtrWidth = DL->getTypeSizeInBits(CE->getType());
            GV.IntVal = GV.IntVal.zextOrTrunc(PtrWidth);
            ASSERT(GV.IntVal.getBitWidth() <= 64, "Bad pointer width");
            GV.PointerVal = PointerTy(uintptr_t(GV.IntVal.getZExtValue()));
            return GV;
        }
        case Instruction::BitCast: {
            GenericValue GV = getConstantValue(Op0);
            llvm::Type* DestTy = CE->getType();
            switch (Op0->getType()->getTypeID()) {
            default: UNREACHABLE("Invalid bitcast operand");
            case llvm::Type::IntegerTyID:
                ASSERT(DestTy->isFloatingPointTy(), "invalid bitcast");
                if (DestTy->isFloatTy())
                    GV.FloatVal = GV.IntVal.bitsToFloat();
                else if (DestTy->isDoubleTy())
                    GV.DoubleVal = GV.IntVal.bitsToDouble();
                break;
            case llvm::Type::FloatTyID:
                ASSERT(DestTy->isIntegerTy(32), "Invalid bitcast");
                GV.IntVal = APInt::floatToBits(GV.FloatVal);
                break;
            case llvm::Type::DoubleTyID:
                ASSERT(DestTy->isIntegerTy(64), "Invalid bitcast");
                GV.IntVal = APInt::doubleToBits(GV.DoubleVal);
                break;
            case llvm::Type::PointerTyID:
                ASSERT(DestTy->isPointerTy(), "Invalid bitcast");
                break; // getConstantValue(Op0)  above already converted it
            }
            return GV;
        }
        case Instruction::Add:
        case Instruction::FAdd:
        case Instruction::Sub:
        case Instruction::FSub:
        case Instruction::Mul:
        case Instruction::FMul:
        case Instruction::UDiv:
        case Instruction::SDiv:
        case Instruction::URem:
        case Instruction::SRem:
        case Instruction::And:
        case Instruction::Or:
        case Instruction::Xor: {
            GenericValue LHS = getConstantValue(Op0);
            GenericValue RHS = getConstantValue(CE->getOperand(1));
            GenericValue GV;
            switch (CE->getOperand(0)->getType()->getTypeID()) {
            default: UNREACHABLE("Bad add llvm::Type!");
            case llvm::Type::IntegerTyID:
                switch (CE->getOpcode()) {
                default: UNREACHABLE("Invalid integer opcode");
                case Instruction::Add: GV.IntVal = LHS.IntVal + RHS.IntVal; break;
                case Instruction::Sub: GV.IntVal = LHS.IntVal - RHS.IntVal; break;
                case Instruction::Mul: GV.IntVal = LHS.IntVal * RHS.IntVal; break;
                case Instruction::UDiv:GV.IntVal = LHS.IntVal.udiv(RHS.IntVal); break;
                case Instruction::SDiv:GV.IntVal = LHS.IntVal.sdiv(RHS.IntVal); break;
                case Instruction::URem:GV.IntVal = LHS.IntVal.urem(RHS.IntVal); break;
                case Instruction::SRem:GV.IntVal = LHS.IntVal.srem(RHS.IntVal); break;
                case Instruction::And: GV.IntVal = LHS.IntVal & RHS.IntVal; break;
                case Instruction::Or:  GV.IntVal = LHS.IntVal | RHS.IntVal; break;
                case Instruction::Xor: GV.IntVal = LHS.IntVal ^ RHS.IntVal; break;
                }
                break;
                case llvm::Type::FloatTyID:
                    switch (CE->getOpcode()) {
                    default: UNREACHABLE("Invalid float opcode");
                    case Instruction::FAdd:
                        GV.FloatVal = LHS.FloatVal + RHS.FloatVal; break;
                    case Instruction::FSub:
                        GV.FloatVal = LHS.FloatVal - RHS.FloatVal; break;
                    case Instruction::FMul:
                        GV.FloatVal = LHS.FloatVal * RHS.FloatVal; break;
                    case Instruction::FDiv:
                        GV.FloatVal = LHS.FloatVal / RHS.FloatVal; break;
                    case Instruction::FRem:
                        GV.FloatVal = std::fmod(LHS.FloatVal,RHS.FloatVal); break;
                    }
                    break;
                    case llvm::Type::DoubleTyID:
                        switch (CE->getOpcode()) {
                        default: UNREACHABLE("Invalid double opcode");
                        case Instruction::FAdd:
                            GV.DoubleVal = LHS.DoubleVal + RHS.DoubleVal; break;
                        case Instruction::FSub:
                            GV.DoubleVal = LHS.DoubleVal - RHS.DoubleVal; break;
                        case Instruction::FMul:
                            GV.DoubleVal = LHS.DoubleVal * RHS.DoubleVal; break;
                        case Instruction::FDiv:
                            GV.DoubleVal = LHS.DoubleVal / RHS.DoubleVal; break;
                        case Instruction::FRem:
                            GV.DoubleVal = std::fmod(LHS.DoubleVal,RHS.DoubleVal); break;
                        }
                        break;
                        case llvm::Type::X86_FP80TyID:
                        case llvm::Type::PPC_FP128TyID:
                        case llvm::Type::FP128TyID: {
                            const fltSemantics &Sem = CE->getOperand(0)->getType()->getFltSemantics();
                            APFloat apfLHS = APFloat(Sem, LHS.IntVal);
                            switch (CE->getOpcode()) {
                            default: UNREACHABLE("Invalid long double opcode");
                            case Instruction::FAdd:
                                apfLHS.add(APFloat(Sem, RHS.IntVal), APFloat::rmNearestTiesToEven);
                                GV.IntVal = apfLHS.bitcastToAPInt();
                                break;
                            case Instruction::FSub:
                                apfLHS.subtract(APFloat(Sem, RHS.IntVal),
                                    APFloat::rmNearestTiesToEven);
                                GV.IntVal = apfLHS.bitcastToAPInt();
                                break;
                            case Instruction::FMul:
                                apfLHS.multiply(APFloat(Sem, RHS.IntVal),
                                    APFloat::rmNearestTiesToEven);
                                GV.IntVal = apfLHS.bitcastToAPInt();
                                break;
                            case Instruction::FDiv:
                                apfLHS.divide(APFloat(Sem, RHS.IntVal),
                                    APFloat::rmNearestTiesToEven);
                                GV.IntVal = apfLHS.bitcastToAPInt();
                                break;
                            case Instruction::FRem:
                                apfLHS.mod(APFloat(Sem, RHS.IntVal),
                                    APFloat::rmNearestTiesToEven);
                                GV.IntVal = apfLHS.bitcastToAPInt();
                                break;
                            }
                        }
                        break;
            }
            return GV;
        }
        default:
            break;
        }

        SmallString<256> Msg;
        raw_svector_ostream OS(Msg);
        OS << "ConstantExpr not handled: " << *CE;
        report_fatal_error(OS.str());
    }

    // Otherwise, we have a simple constant.
    GenericValue Result;
    switch (C->getType()->getTypeID()) {
    case llvm::Type::FloatTyID:
        Result.FloatVal = cast<ConstantFP>(C)->getValueAPF().convertToFloat();
        break;
    case llvm::Type::DoubleTyID:
        Result.DoubleVal = cast<ConstantFP>(C)->getValueAPF().convertToDouble();
        break;
    case llvm::Type::X86_FP80TyID:
    case llvm::Type::FP128TyID:
    case llvm::Type::PPC_FP128TyID:
        Result.IntVal = cast <ConstantFP>(C)->getValueAPF().bitcastToAPInt();
        break;
    case llvm::Type::IntegerTyID:
        Result.IntVal = cast<ConstantInt>(C)->getValue();
        break;
    case llvm::Type::PointerTyID:
        if (isa<ConstantPointerNull>(C))
            Result.PointerVal = nullptr;
        else if (const Function *F = dyn_cast<Function>(C))
            Result.PointerVal =
                getPointerToFunction(
                    const_cast<Function*>(F),
                    DL->getPointerSizeInBits()/8
                );
        else if (const GlobalVariable *GV = dyn_cast<GlobalVariable>(C))
            Result.PointerVal =
                getPointerToGlobal(
                    const_cast<GlobalVariable*>(GV),
                    DL->getTypeAllocSize(GV->getType()->getPointerElementType()),
                    0
                );
        else if (const BlockAddress *BA = dyn_cast<BlockAddress>(C))
            Result.PointerVal =
                getPointerBasicBlock(BA->getBasicBlock(), DL->getPointerSizeInBits()/8);
        else
            UNREACHABLE("Unknown constant pointer llvm::Type!");
        break;
    case llvm::Type::VectorTyID: {
        unsigned elemNum;
        llvm::Type* ElemTy;
        const ConstantDataVector *CDV = dyn_cast<ConstantDataVector>(C);
        const ConstantVector *CV = dyn_cast<ConstantVector>(C);
        const ConstantAggregateZero *CAZ = dyn_cast<ConstantAggregateZero>(C);

        if (CDV) {
            elemNum = CDV->getNumElements();
            ElemTy = CDV->getElementType();
        } else if (CV || CAZ) {
            VectorType* VTy = dyn_cast<VectorType>(C->getType());
            elemNum = VTy->getNumElements();
            ElemTy = VTy->getElementType();
        } else {
            UNREACHABLE("Unknown constant vector llvm::Type!");
        }

        Result.AggregateVal.resize(elemNum);
        // Check if vector holds floats.
        if(ElemTy->isFloatTy()) {
            if (CAZ) {
                GenericValue floatZero;
                floatZero.FloatVal = 0.f;
                std::fill(Result.AggregateVal.begin(), Result.AggregateVal.end(),
                    floatZero);
                break;
            }
            if(CV) {
                for (unsigned i = 0; i < elemNum; ++i)
                    if (!isa<UndefValue>(CV->getOperand(i)))
                        Result.AggregateVal[i].FloatVal = cast<ConstantFP>(
                            CV->getOperand(i))->getValueAPF().convertToFloat();
                break;
            }
            if(CDV)
                for (unsigned i = 0; i < elemNum; ++i)
                    Result.AggregateVal[i].FloatVal = CDV->getElementAsFloat(i);

            break;
        }
        // Check if vector holds doubles.
        if (ElemTy->isDoubleTy()) {
            if (CAZ) {
                GenericValue doubleZero;
                doubleZero.DoubleVal = 0.0;
                std::fill(Result.AggregateVal.begin(), Result.AggregateVal.end(),
                    doubleZero);
                break;
            }
            if(CV) {
                for (unsigned i = 0; i < elemNum; ++i)
                    if (!isa<UndefValue>(CV->getOperand(i)))
                        Result.AggregateVal[i].DoubleVal = cast<ConstantFP>(
                            CV->getOperand(i))->getValueAPF().convertToDouble();
                break;
            }
            if(CDV)
                for (unsigned i = 0; i < elemNum; ++i)
                    Result.AggregateVal[i].DoubleVal = CDV->getElementAsDouble(i);

            break;
        }
        // Check if vector holds integers.
        if (ElemTy->isIntegerTy()) {
            if (CAZ) {
                GenericValue intZero;
                intZero.IntVal = APInt(ElemTy->getScalarSizeInBits(), 0ull);
                std::fill(Result.AggregateVal.begin(), Result.AggregateVal.end(),
                    intZero);
                break;
            }
            if(CV) {
                for (unsigned i = 0; i < elemNum; ++i)
                    if (!isa<UndefValue>(CV->getOperand(i)))
                        Result.AggregateVal[i].IntVal = cast<ConstantInt>(
                            CV->getOperand(i))->getValue();
                    else {
                        Result.AggregateVal[i].IntVal =
                            APInt(CV->getOperand(i)->getType()->getPrimitiveSizeInBits(), 0);
                    }
                break;
            }
            if(CDV)
                for (unsigned i = 0; i < elemNum; ++i)
                    Result.AggregateVal[i].IntVal = APInt(
                        CDV->getElementType()->getPrimitiveSizeInBits(),
                        CDV->getElementAsInteger(i));

            break;
        }
        UNREACHABLE("Unknown constant pointer llvm::Type!");
    }
    break;

    default:
        SmallString<256> Msg;
        raw_svector_ostream OS(Msg);
        OS << "ERROR: Constant unimplemented for llvm::Type: " << *C->getType();
        report_fatal_error(OS.str());
    }

    return Result;
}

void MemorySimulator::initializeMemory(const llvm::Constant *Init, void *Addr) {
    TRACE_FUNC;
    using namespace llvm;

    if (isa<llvm::UndefValue>(Init))
        return;

    if (const llvm::ConstantVector *CP = llvm::dyn_cast<llvm::ConstantVector>(Init)) {
        unsigned ElementSize =
            pimpl_->DL->getTypeAllocSize(CP->getType()->getElementType());
        for (unsigned i = 0, e = CP->getNumOperands(); i != e; ++i)
            initializeMemory(CP->getOperand(i), (char*) Addr + i * ElementSize);
        return;
    }

    if (isa<ConstantAggregateZero>(Init)) {
        memset(Addr, 0, (size_t) pimpl_->DL->getTypeAllocSize(Init->getType()));
        return;
    }

    if (const ConstantArray *CPA = dyn_cast<ConstantArray>(Init)) {
        unsigned ElementSize = pimpl_->DL->getTypeAllocSize(CPA->getType()->getElementType());
        for (unsigned i = 0, e = CPA->getNumOperands(); i != e; ++i)
            initializeMemory(CPA->getOperand(i), (char*) Addr + i * ElementSize);
        return;
    }

    if (const ConstantStruct *CPS = dyn_cast<ConstantStruct>(Init)) {
        const StructLayout *SL =
            pimpl_->DL->getStructLayout(cast<StructType>(CPS->getType()));
        for (unsigned i = 0, e = CPS->getNumOperands(); i != e; ++i)
            initializeMemory(CPS->getOperand(i), (char*) Addr + SL->getElementOffset(i));
        return;
    }

    if (const ConstantDataSequential *CDS = dyn_cast<ConstantDataSequential>(Init)) {
        // CDS is already laid out in host memory order.
        StringRef Data = CDS->getRawDataValues();
        memcpy(Addr, Data.data(), Data.size());
        return;
    }

    if (Init->getType()->isFirstClassType()) {
        TRACE_FUNC;
        GenericValue Val = getConstantValue(Init);
        auto Ty = Init->getType();

        const unsigned StoreBytes = pimpl_->DL->getTypeStoreSize(Ty);
        auto buffer = mutable_buffer_t{(uint8_t*)Addr, StoreBytes};

        switch (Ty->getTypeID()) {
        default:
            // XXX: support vectors?
            throw std::logic_error( "Cannot store value of llvm::Type " + util::toString(*Ty) + "!");
            break;
        case llvm::Type::IntegerTyID:
            StoreIntToMemory(Val.IntVal, buffer);
            break;
        case llvm::Type::FloatTyID:
            StoreBytesToMemory(bufferOfPod(Val.FloatVal, StoreBytes), buffer);
            break;
        case llvm::Type::DoubleTyID:
            StoreBytesToMemory(bufferOfPod(Val.DoubleVal, StoreBytes), buffer);
            break;
        case llvm::Type::X86_FP80TyID:
            StoreIntToMemory(Val.IntVal, buffer);
            break;
        case llvm::Type::PointerTyID:
            StoreBytesToMemory(bufferOfPod(Val.PointerVal, StoreBytes), buffer);
            break;
        }

        return;
    }

}

llvm::Function* MemorySimulator::accessFunction(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto result = util::at(pimpl_->constantsBwd, realPtr);
    if(!result) signalIllegalLoad(realPtr);

    return llvm::dyn_cast<llvm::Function>(result.getUnsafe());
}
llvm::BasicBlock* MemorySimulator::accessBasicBlock(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto result = util::at(pimpl_->constantsBwd, realPtr);
    if(!result) signalIllegalLoad(realPtr);

    return llvm::dyn_cast<llvm::BasicBlock>(result.getUnsafe());
}
std::pair<llvm::GlobalValue*, SimulatedPtrSize> MemorySimulator::accessGlobal(void* p) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(p);
    TRACE_PARAM(realPtr);

    if(realPtr > pimpl_->constantEnd || realPtr < pimpl_->constantStart) signalIllegalLoad(realPtr);
    auto lb = util::less_or_equal(pimpl_->constantsBwd, realPtr);

    if(lb == pimpl_->constantsBwd.end()) signalIllegalLoad(realPtr);

    return { llvm::dyn_cast<llvm::GlobalValue>(lb->second), realPtr - lb->first };
}

static SimulatedPtrSize calc_real_memory_amount(SimulatedPtrSize amount, SimulatedPtrSize chunk_size) {
    TRACE_FUNC;
    static const auto max_chunk_size = SimulatedPtrSize(1) << (8 * sizeof(SimulatedPtrSize) - 1);
    if(amount >= max_chunk_size) signalOutOfMemory(amount);

    dbgs() << tfm::format("amount = %s", amount) << endl;
    dbgs() << tfm::format("chunk_size = %s", chunk_size) << endl;
    while(amount > chunk_size) {
        chunk_size *= 2;
        dbgs() << tfm::format("chunk_size = %s", chunk_size) << endl;
    }
    return chunk_size;
}

void* MemorySimulator::AllocateMemory(SimulatedPtrSize amount) {
    TRACE_FUNC;
    const auto real_amount = calc_real_memory_amount(amount, pimpl_->tree.chunk_size);

    SimulatedPtr ptr;
    if(pimpl_->currentAllocOffset % real_amount == 0) {
        ptr = pimpl_->allocStart + pimpl_->currentAllocOffset;
    } else {
        ptr = pimpl_->allocStart + (pimpl_->currentAllocOffset / real_amount + 1) * real_amount;
    }

    pimpl_->tree.allocate(ptr, amount, SegmentNode::MemoryState::Uninit, SegmentNode::MemoryStatus::Alloca);

    pimpl_->currentAllocOffset = ptr + real_amount - pimpl_->allocStart;

    return ptr_cast(ptr);
}

void MemorySimulator::DeallocateMemory(void* ptr) {
    TRACE_FUNC;

    auto realPtr = ptr_cast(ptr);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalFree(realPtr);

    pimpl_->tree.free(realPtr, SegmentNode::MemoryStatus::Alloca);
}

void* MemorySimulator::MallocMemory(SimulatedPtrSize amount, MallocFill fillWith) {
    TRACE_FUNC;
    const auto real_amount = calc_real_memory_amount(amount, pimpl_->tree.chunk_size);
    const auto memState = (fillWith == MallocFill::ZERO) ? SegmentNode::MemoryState::Memset : SegmentNode::MemoryState::Uninit;

    SimulatedPtr ptr;
    if(pimpl_->currentMallocOffset % real_amount == 0) {
        ptr = pimpl_->mallocStart + pimpl_->currentMallocOffset;
    } else {
        ptr = pimpl_->mallocStart + (pimpl_->currentMallocOffset / real_amount + 1) * real_amount;
    }

    pimpl_->tree.allocate(ptr, amount, memState, SegmentNode::MemoryStatus::Malloc);
    pimpl_->currentMallocOffset = ptr + real_amount - pimpl_->mallocStart;

    return ptr_cast(ptr);
}

void MemorySimulator::FreeMemory(void* ptr) {
    TRACE_FUNC;

    auto realPtr = ptr_cast(ptr);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalFree(realPtr);

    pimpl_->tree.free(realPtr, SegmentNode::MemoryStatus::Malloc);
}

static void assign(MemorySimulator::mutable_buffer_t dst, MemorySimulator::buffer_t src) {
    TRACE_FUNC;
    ASSERTC(dst.size() == src.size());
    std::copy(std::begin(src), std::end(src), std::begin(dst));
}

auto MemorySimulator::LoadBytesFromMemory(mutable_buffer_t buffer, buffer_t where) -> ValueState {
    TRACE_FUNC;
    ASSERTC(buffer.size() == where.size());

    const auto size = where.size();
    const auto chunk_size = pimpl_->tree.chunk_size;
    auto ptr = ptr_cast(where.data());
    TRACE_PARAM(ptr);
    if(!pimpl_->isAPointer(ptr)) signalIllegalLoad(ptr);

    if(pimpl_->isFromGlobalSpace(ptr)) {
        auto&& gv = accessGlobal(const_cast<void*>(static_cast<const void*>(where.data())));


        auto actualMemory = pimpl_->getPointerToGlobalMemory(this, gv.first, size, gv.second);
        if(!actualMemory) signalIllegalLoad(ptr);

        std::memcpy(buffer.data(), actualMemory, size);
        TRACE_PARAM(hex(buffer));
        return ValueState::CONCRETE;
    }

    auto offset = ptr - pimpl_->tree.start;
    auto loaded = (SimulatedPtrSize)0;


    while(loaded < size) {
        const auto current = pimpl_->tree.get(ptr);

        auto current_size = chunk_size;
        // if we start in the middle of a chunk
        current_size -= offset % chunk_size;
        // if we end in the middle of a chunk
        if(size - loaded < chunk_size) {
            auto leftover = size - loaded - offset%chunk_size;
            current_size -= (chunk_size - leftover);
        }

        auto slice = buffer.slice(loaded, current_size);
        if(current.memState == SegmentNode::MemoryState::Memset) {
            std::memset(slice.data(), current.filledWith, slice.size());
        } else if(current.memState == SegmentNode::MemoryState::Uninit) {
            return ValueState::UNKNOWN;
        } else {
            assign(slice, buffer_t{current.data, current_size});
        }

        loaded += current_size;
        offset += current_size;
        ptr += current_size;
    }

    return ValueState::CONCRETE;
    // XXX: what about endianness?
}
void MemorySimulator::StoreBytesToMemory(buffer_t buffer, mutable_buffer_t where) {
    TRACE_FUNC;

    ASSERTC(buffer.size() == where.size());
    const auto Ptr = where.data();
    const auto realPtr = ptr_cast(Ptr);

    TRACE_PARAM(realPtr);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalStore(realPtr);
    const auto size = where.size();
    const auto Src = buffer.data();

    TRACE_PARAM(hex(buffer));

    if(pimpl_->isFromGlobalSpace(realPtr)) {
        auto&& gv = accessGlobal(const_cast<void*>(static_cast<const void*>(where.data())));
        auto ptr = pimpl_->getPointerToGlobalMemory(this, gv.first, size, gv.second);
        if(!ptr) signalIllegalStore(realPtr);
        std::memcpy(ptr, Src, size);
        return;
    }

    pimpl_->tree.store(realPtr, Src, size);
    // XXX: what about endianness?
}

void MemorySimulator::StoreIntToMemory(const llvm::APInt& IntVal, mutable_buffer_t where) {
    TRACE_FUNC;
    const auto Ptr = where.data();
    auto size = where.size();
    const uint8_t* Src = (const uint8_t*) IntVal.getRawData();
    const auto realPtr = ptr_cast(Ptr);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalStore(realPtr);

    if(llvm::sys::IsLittleEndianHost) {
        // Little-endian host - the source is ordered from LSB to MSB
        // Order the destination from LSB to MSB
        // => Do a straight copy
        StoreBytesToMemory(buffer_t{Src, size}, mutable_buffer_t{Ptr, size});
    } else throw std::logic_error("big-endian hosts are not supported");
}

auto MemorySimulator::LoadIntFromMemory(llvm::APInt& val, buffer_t where) -> ValueState {
    TRACE_FUNC;
    const auto Ptr = where.data();
    auto size = where.size();
    const auto realPtr = ptr_cast(Ptr);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalLoad(realPtr);

    const auto chunk_size = pimpl_->tree.chunk_size;

    ASSERTC(size <= chunk_size);

    // TODO: belyaev Think!
    if(realPtr / chunk_size != (realPtr + size -1) / chunk_size) {
        // if the piece crosses the chunk border
        TRACE_FMT("While loading from 0x%x", realPtr);
        TRACE_FMT("Chunk size = %d", chunk_size);
        signalUnsupported(realPtr);
    }
    uint8_t* Dst = reinterpret_cast<uint8_t*>(const_cast<uint64_t*>(val.getRawData()));

    if(pimpl_->isFromGlobalSpace(realPtr)) {
        auto&& gv = accessGlobal(const_cast<void*>(static_cast<const void*>(where.data())));
        auto ptr = pimpl_->getPointerToGlobalMemory(this, gv.first, size, gv.second);
        if(!ptr) signalIllegalLoad(realPtr);
        std::memcpy(Dst, ptr, size);
        return ValueState::CONCRETE;
    }

    auto&& load = pimpl_->tree.get(realPtr);

    uint8_t* Src = load.data;

    if(load.memState == SegmentNode::MemoryState::Memset) {
        std::memset(Dst, load.filledWith , size);
        return ValueState::CONCRETE;
    } else if(load.memState == SegmentNode::MemoryState::Uninit) {
        return ValueState::UNKNOWN; // FIXME: implement uninit handling policy
    }

    if(llvm::sys::IsLittleEndianHost)
        // Little-endian host - the source is ordered from LSB to MSB
        // Order the destination from LSB to MSB
        // => Do a straight copy
        std::memcpy(Dst, Src, size);
    else {
        // Big-endian host - the source is an array of 64 bit words ordered from LSW to MSW
        // Each word is ordered from MSB to LSB
        // Order the destination from MSB to LSB
        // => Reverse the word order, but not the bytes in a word
        while(size > sizeof(uint64_t)) {
            size -= sizeof(uint64_t);
            // May not be aligned so use memcpy
            std::memcpy(Dst, Src + size, sizeof(uint64_t));
            Dst += sizeof(uint64_t);
        }

        std::memcpy(Dst + sizeof(uint64_t) - size, Src, size);
    }

    TRACE_FMT("Loaded value %s", val.getLimitedValue());
    return ValueState::CONCRETE;
}

void* MemorySimulator::MemChr(void* ptr, uint8_t ch, size_t limit) {
    TRACE_FUNC;
    auto realPtr = ptr_cast(ptr);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalLoad(realPtr);

    if(pimpl_->isFromGlobalSpace(realPtr)) {
        auto&& gv = accessGlobal(const_cast<void*>(static_cast<const void*>(ptr)));
        auto ptr = pimpl_->getPointerToGlobalMemory(this, gv.first, 1, gv.second);
        auto ret = std::memchr(ptr, ch, limit);
        if(!ret) return nullptr;
        return ptr_cast(realPtr + ptrSub(ret, ptr));
    }

    auto ret = pimpl_->tree.memchr(ptr_cast(ptr), ch, limit);
    return ptr_cast(ret);
}

void MemorySimulator::Memset(void* dst, uint8_t fill, size_t size) {
    auto realPtr = ptr_cast(dst);
    if(!pimpl_->isAPointer(realPtr)) signalIllegalStore(realPtr);

    pimpl_->tree.memset(realPtr, fill, size);
}

MemorySimulator::MemorySimulator(const llvm::DataLayout& dl) : pimpl_{new Impl{dl}} {}

MemorySimulator::~MemorySimulator() {}

} /* namespace borealis */

#include "Util/unmacros.h"
