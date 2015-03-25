//===-- Execution.cpp - Implement code to simulate the program ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file contains the actual instruction interpreter.
//
//===----------------------------------------------------------------------===//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/CodeGen/IntrinsicLowering.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/ExecutionEngine/GenericValue.h>

#include <algorithm>
#include <cmath>


#include "Executor/ExecutionEngine.h"
#include "Executor/Exceptions.h"

#include "Util/util.h"

#include "Logging/tracer.hpp"

using namespace borealis;
using namespace llvm;

#include "Util/macros.h"

using byte = uint8_t;
using buffer_t = llvm::ArrayRef<byte>;
using mutable_buffer_t = llvm::MutableArrayRef<byte>;

template<class T>
static mutable_buffer_t bufferOfPod(T& value, size_t bytes) {
    TRACE_FUNC;
    return mutable_buffer_t{ static_cast<byte*>(static_cast<void*>(&value)), bytes };
}

template<class T>
static buffer_t bufferOfPod(const T& value, size_t bytes) {
    TRACE_FUNC;
    return buffer_t{ static_cast<const byte*>(static_cast<const void*>(&value)), bytes };
}

void borealis::ExecutionEngine::StoreValueToMemory(const llvm::GenericValue &Val, byte* Ptr, llvm::Type *Ty) {
    TRACE_FUNC;
    const unsigned StoreBytes = getDataLayout()->getTypeStoreSize(Ty);
    auto buffer = mutable_buffer_t{Ptr, StoreBytes};

    switch (Ty->getTypeID()) {
    default:
        // XXX: support vectors?
        throw std::logic_error( "Cannot store value of llvm::Type " + util::toString(*Ty) + "!");
        break;
    case llvm::Type::IntegerTyID:
        Mem.StoreIntToMemory(Val.IntVal, buffer);
        break;
    case llvm::Type::FloatTyID:
        Mem.StoreBytesToMemory(bufferOfPod(Val.FloatVal, StoreBytes), buffer);
        break;
    case llvm::Type::DoubleTyID:
        Mem.StoreBytesToMemory(bufferOfPod(Val.DoubleVal, StoreBytes), buffer);
        break;
    case llvm::Type::X86_FP80TyID:
        Mem.StoreIntToMemory(Val.IntVal, buffer);
        break;
    case llvm::Type::PointerTyID:
        Mem.StoreBytesToMemory(bufferOfPod(Val.PointerVal, StoreBytes), buffer);
        break;
    }

// FIXME: do this with incoming data
//    if (sys::IsLittleEndianHost != getDataLayout()->isLittleEndian())
//        // Host and target are different endian - reverse the stored bytes.
//        std::reverse((uint8_t*)Ptr, StoreBytes + (uint8_t*)Ptr);

}

void borealis::ExecutionEngine::LoadValueFromMemory(llvm::GenericValue &Result, const byte* Ptr, llvm::Type* Ty) {
    TRACE_FUNC;
    const unsigned LoadBytes = getDataLayout()->getTypeStoreSize(Ty);
    auto buffer = buffer_t{Ptr, LoadBytes};

    switch (Ty->getTypeID()) {
    case llvm::Type::IntegerTyID:
        // An APInt with all words initially zero.
        Result.IntVal = APInt(cast<IntegerType>(Ty)->getBitWidth(), 0);
        Mem.LoadIntFromMemory(Result.IntVal, buffer);
        break;
    case llvm::Type::FloatTyID:
        Mem.LoadBytesFromMemory(bufferOfPod(Result.FloatVal, LoadBytes), buffer);
        break;
    case llvm::Type::DoubleTyID:
        Mem.LoadBytesFromMemory(bufferOfPod(Result.DoubleVal, LoadBytes), buffer);
        break;
    case llvm::Type::PointerTyID:
        Mem.LoadBytesFromMemory(bufferOfPod(Result.PointerVal, LoadBytes), buffer);
        break;
    case llvm::Type::X86_FP80TyID: {
        // This is endian dependent, but it will only work on x86 anyway.
        // FIXME: Will not trap if loading a signaling NaN.
        Result.IntVal = APInt(cast<IntegerType>(Ty)->getBitWidth(), 0);
        Mem.LoadIntFromMemory(Result.IntVal, buffer);
        break;
    }
    default:
        throw std::logic_error( "Cannot load value of llvm::Type " + util::toString(*Ty) + "!");
        break;
    }
}

//===----------------------------------------------------------------------===//
//                     Various Helper Functions
//===----------------------------------------------------------------------===//

static void SetValue(Value *V, GenericValue Val, ExecutorContext &SF) {
    SF.Values[V] = Val;
}

//===----------------------------------------------------------------------===//
//                    Binary Instruction Implementations
//===----------------------------------------------------------------------===//

#define IMPLEMENT_BINARY_OPERATOR(OP, TY) \
    case llvm::Type::TY##TyID: \
    Dest.TY##Val = Src1.TY##Val OP Src2.TY##Val; \
    break

void borealis::ExecutionEngine::executeFAddInst(GenericValue &Dest, GenericValue Src1,
        GenericValue Src2, llvm::Type *Ty) {
    TRACE_FUNC;
    switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(+, Float);
    IMPLEMENT_BINARY_OPERATOR(+, Double);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FAdd instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
}

void borealis::ExecutionEngine::executeFSubInst(GenericValue &Dest, GenericValue Src1,
    GenericValue Src2, llvm::Type *Ty) {
    TRACE_FUNC;
    switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(-, Float);
    IMPLEMENT_BINARY_OPERATOR(-, Double);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FSub instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
}

void borealis::ExecutionEngine::executeFMulInst(GenericValue &Dest, GenericValue Src1,
    GenericValue Src2, llvm::Type *Ty) {
    TRACE_FUNC;
    switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(*, Float);
    IMPLEMENT_BINARY_OPERATOR(*, Double);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FMul instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
}

void borealis::ExecutionEngine::executeFDivInst(GenericValue &Dest, GenericValue Src1,
    GenericValue Src2, llvm::Type *Ty) {
    TRACE_FUNC;
    switch (Ty->getTypeID()) {
    IMPLEMENT_BINARY_OPERATOR(/, Float);
    IMPLEMENT_BINARY_OPERATOR(/, Double);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FDiv instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
}

void borealis::ExecutionEngine::executeFRemInst(GenericValue &Dest, GenericValue Src1,
    GenericValue Src2, llvm::Type *Ty) {
    TRACE_FUNC;
    switch (Ty->getTypeID()) {
    case llvm::Type::FloatTyID:
        Dest.FloatVal = fmod(Src1.FloatVal, Src2.FloatVal);
        break;
    case llvm::Type::DoubleTyID:
        Dest.DoubleVal = fmod(Src1.DoubleVal, Src2.DoubleVal);
        break;
    default:
        borealis::dbgs() << "Unhandled llvm::Type for Rem instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
}

#define IMPLEMENT_INTEGER_ICMP(OP, TY) \
    case llvm::Type::IntegerTyID:  \
    Dest.IntVal = APInt(1,Src1.IntVal.OP(Src2.IntVal)); \
    break;

#define IMPLEMENT_VECTOR_INTEGER_ICMP(OP, TY)                        \
    case llvm::Type::VectorTyID: {                                           \
        ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());    \
        Dest.AggregateVal.resize( Src1.AggregateVal.size() );            \
        for( uint32_t _i=0;_i<Src1.AggregateVal.size();_i++)             \
        Dest.AggregateVal[_i].IntVal = APInt(1,                        \
            Src1.AggregateVal[_i].IntVal.OP(Src2.AggregateVal[_i].IntVal));\
    } break;

// Handle pointers specially because they must be compared with only as much
// width as the host has.  We _do not_ want to be comparing 64 bit values when
// running on a 32-bit target, otherwise the upper 32 bits might mess up
// comparisons if they contain garbage.
#define IMPLEMENT_POINTER_ICMP(OP) \
    case llvm::Type::PointerTyID: \
    Dest.IntVal = APInt(1,(void*)(intptr_t)Src1.PointerVal OP \
        (void*)(intptr_t)Src2.PointerVal); \
        break;

GenericValue borealis::ExecutionEngine::executeICMP_EQ(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(eq,Ty);
    IMPLEMENT_POINTER_ICMP(==);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_EQ predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_NE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ne,Ty);
    IMPLEMENT_POINTER_ICMP(!=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_NE predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_ULT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ult,Ty);
    IMPLEMENT_POINTER_ICMP(<);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_ULT predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_SLT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(slt,Ty);
    IMPLEMENT_POINTER_ICMP(<);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_SLT predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_UGT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ugt,Ty);
    IMPLEMENT_POINTER_ICMP(>);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_UGT predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_SGT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sgt,Ty);
    IMPLEMENT_POINTER_ICMP(>);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_SGT predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_ULE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(ule,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_ULE predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_SLE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sle,Ty);
    IMPLEMENT_POINTER_ICMP(<=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_SLE predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_UGE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(uge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_UGE predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeICMP_SGE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_VECTOR_INTEGER_ICMP(sge,Ty);
    IMPLEMENT_POINTER_ICMP(>=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for ICMP_SGE predicate: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

void InstructionExecutor::visitICmpInst(ICmpInst &I) {
    TRACE_FUNC;
    ExecutorContext& SF = ee->getCurrentContext();
    llvm::Type *Ty    = I.getOperand(0)->getType();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue R;   // Result

    switch (I.getPredicate()) {
    case ICmpInst::ICMP_EQ:  R = ee->executeICMP_EQ(Src1,  Src2, Ty); break;
    case ICmpInst::ICMP_NE:  R = ee->executeICMP_NE(Src1,  Src2, Ty); break;
    case ICmpInst::ICMP_ULT: R = ee->executeICMP_ULT(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_SLT: R = ee->executeICMP_SLT(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_UGT: R = ee->executeICMP_UGT(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_SGT: R = ee->executeICMP_SGT(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_ULE: R = ee->executeICMP_ULE(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_SLE: R = ee->executeICMP_SLE(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_UGE: R = ee->executeICMP_UGE(Src1, Src2, Ty); break;
    case ICmpInst::ICMP_SGE: R = ee->executeICMP_SGE(Src1, Src2, Ty); break;
    default:
        borealis::dbgs() << "Don't know how to handle this ICmp predicate!\n-->" << I;
        UNREACHABLE(nullptr);
    }

    SetValue(&I, R, SF);
}

#define IMPLEMENT_FCMP(OP, TY) \
    case llvm::Type::TY##TyID: \
    Dest.IntVal = APInt(1,Src1.TY##Val OP Src2.TY##Val); \
    break

#define IMPLEMENT_VECTOR_FCMP_T(OP, TY)                             \
    ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());     \
    Dest.AggregateVal.resize( Src1.AggregateVal.size() );             \
    for( uint32_t _i=0;_i<Src1.AggregateVal.size();_i++)              \
    Dest.AggregateVal[_i].IntVal = APInt(1,                         \
        Src1.AggregateVal[_i].TY##Val OP Src2.AggregateVal[_i].TY##Val);\
        break;

#define IMPLEMENT_VECTOR_FCMP(OP)                                   \
    case llvm::Type::VectorTyID:                                            \
    if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {   \
        IMPLEMENT_VECTOR_FCMP_T(OP, Float);                           \
    } else {                                                        \
        IMPLEMENT_VECTOR_FCMP_T(OP, Double);                        \
    }

GenericValue borealis::ExecutionEngine::executeFCMP_OEQ(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(==, Float);
    IMPLEMENT_FCMP(==, Double);
    IMPLEMENT_VECTOR_FCMP(==);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FCmp EQ instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

#define IMPLEMENT_SCALAR_NANS(TY, X,Y)                                      \
    if (TY->isFloatTy()) {                                                    \
        if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {             \
            Dest.IntVal = APInt(1,false);                                         \
            return Dest;                                                          \
        }                                                                       \
    } else {                                                                  \
        if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) {         \
            Dest.IntVal = APInt(1,false);                                         \
            return Dest;                                                          \
        }                                                                       \
    }

#define MASK_VECTOR_NANS_T(X,Y, TZ, FLAG)                                   \
    ASSERTC(X.AggregateVal.size() == Y.AggregateVal.size());                   \
    Dest.AggregateVal.resize( X.AggregateVal.size() );                        \
    for( uint32_t _i=0;_i<X.AggregateVal.size();_i++) {                       \
        if (X.AggregateVal[_i].TZ##Val != X.AggregateVal[_i].TZ##Val ||         \
            Y.AggregateVal[_i].TZ##Val != Y.AggregateVal[_i].TZ##Val)           \
            Dest.AggregateVal[_i].IntVal = APInt(1,FLAG);                         \
            else  {                                                                 \
                Dest.AggregateVal[_i].IntVal = APInt(1,!FLAG);                        \
            }                                                                       \
    }

#define MASK_VECTOR_NANS(TY, X,Y, FLAG)                                     \
    if (TY->isVectorTy()) {                                                   \
        if (dyn_cast<VectorType>(TY)->getElementType()->isFloatTy()) {          \
            MASK_VECTOR_NANS_T(X, Y, Float, FLAG)                                 \
        } else {                                                                \
            MASK_VECTOR_NANS_T(X, Y, Double, FLAG)                                \
        }                                                                       \
    }                                                                         \



GenericValue borealis::ExecutionEngine::executeFCMP_ONE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty)
{
    GenericValue Dest;
    // if input is scalar value and Src1 or Src2 is NaN return false
    IMPLEMENT_SCALAR_NANS(Ty, Src1, Src2)
    // if vector input detect NaNs and fill mask
    MASK_VECTOR_NANS(Ty, Src1, Src2, false)
    GenericValue DestMask = Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(!=, Float);
    IMPLEMENT_FCMP(!=, Double);
    IMPLEMENT_VECTOR_FCMP(!=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FCmp NE instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    // in vector case mask out NaN elements
    if (Ty->isVectorTy())
        for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)
            if (DestMask.AggregateVal[_i].IntVal == false)
                Dest.AggregateVal[_i].IntVal = APInt(1,false);

    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFCMP_OLE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<=, Float);
    IMPLEMENT_FCMP(<=, Double);
    IMPLEMENT_VECTOR_FCMP(<=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FCmp LE instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFCMP_OGE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>=, Float);
    IMPLEMENT_FCMP(>=, Double);
    IMPLEMENT_VECTOR_FCMP(>=);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FCmp GE instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFCMP_OLT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(<, Float);
    IMPLEMENT_FCMP(<, Double);
    IMPLEMENT_VECTOR_FCMP(<);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FCmp LT instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFCMP_OGT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    GenericValue Dest;
    switch (Ty->getTypeID()) {
    IMPLEMENT_FCMP(>, Float);
    IMPLEMENT_FCMP(>, Double);
    IMPLEMENT_VECTOR_FCMP(>);
    default:
        borealis::dbgs() << "Unhandled llvm::Type for FCmp GT instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }
    return Dest;
}

#define IMPLEMENT_UNORDERED(TY, X,Y)                                     \
    if (TY->isFloatTy()) {                                                 \
        if (X.FloatVal != X.FloatVal || Y.FloatVal != Y.FloatVal) {          \
            Dest.IntVal = APInt(1,true);                                       \
            return Dest;                                                       \
        }                                                                    \
    } else if (X.DoubleVal != X.DoubleVal || Y.DoubleVal != Y.DoubleVal) { \
        Dest.IntVal = APInt(1,true);                                         \
        return Dest;                                                         \
    }

#define IMPLEMENT_VECTOR_UNORDERED(TY, X,Y, _FUNC)                       \
    if (TY->isVectorTy()) {                                                \
        GenericValue DestMask = Dest;                                        \
        Dest = _FUNC(Src1, Src2, Ty);                                        \
        for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)               \
        if (DestMask.AggregateVal[_i].IntVal == true)                    \
        Dest.AggregateVal[_i].IntVal = APInt(1,true);                  \
        return Dest;                                                       \
    }

GenericValue borealis::ExecutionEngine::executeFCMP_UEQ(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OEQ)
    return executeFCMP_OEQ(Src1, Src2, Ty);

}

GenericValue borealis::ExecutionEngine::executeFCMP_UNE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_ONE)
    return executeFCMP_ONE(Src1, Src2, Ty);
}

GenericValue borealis::ExecutionEngine::executeFCMP_ULE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLE)
    return executeFCMP_OLE(Src1, Src2, Ty);
}

GenericValue borealis::ExecutionEngine::executeFCMP_UGE(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGE)
    return executeFCMP_OGE(Src1, Src2, Ty);
}

GenericValue borealis::ExecutionEngine::executeFCMP_ULT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OLT)
    return executeFCMP_OLT(Src1, Src2, Ty);
}

GenericValue borealis::ExecutionEngine::executeFCMP_UGT(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    IMPLEMENT_UNORDERED(Ty, Src1, Src2)
    MASK_VECTOR_NANS(Ty, Src1, Src2, true)
    IMPLEMENT_VECTOR_UNORDERED(Ty, Src1, Src2, executeFCMP_OGT)
    return executeFCMP_OGT(Src1, Src2, Ty);
}

GenericValue borealis::ExecutionEngine::executeFCMP_ORD(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    if(Ty->isVectorTy()) {
        ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());
        Dest.AggregateVal.resize( Src1.AggregateVal.size() );
        if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
            for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
                Dest.AggregateVal[_i].IntVal = APInt(1,
                    ( (Src1.AggregateVal[_i].FloatVal ==
                        Src1.AggregateVal[_i].FloatVal) &&
                        (Src2.AggregateVal[_i].FloatVal ==
                            Src2.AggregateVal[_i].FloatVal)));
        } else {
            for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
                Dest.AggregateVal[_i].IntVal = APInt(1,
                    ( (Src1.AggregateVal[_i].DoubleVal ==
                        Src1.AggregateVal[_i].DoubleVal) &&
                        (Src2.AggregateVal[_i].DoubleVal ==
                            Src2.AggregateVal[_i].DoubleVal)));
        }
    } else if (Ty->isFloatTy())
        Dest.IntVal = APInt(1,(Src1.FloatVal == Src1.FloatVal &&
            Src2.FloatVal == Src2.FloatVal));
    else {
        Dest.IntVal = APInt(1,(Src1.DoubleVal == Src1.DoubleVal &&
            Src2.DoubleVal == Src2.DoubleVal));
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFCMP_UNO(GenericValue Src1, GenericValue Src2,
    llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    if(Ty->isVectorTy()) {
        ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());
        Dest.AggregateVal.resize( Src1.AggregateVal.size() );
        if(dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy()) {
            for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
                Dest.AggregateVal[_i].IntVal = APInt(1,
                    ( (Src1.AggregateVal[_i].FloatVal !=
                        Src1.AggregateVal[_i].FloatVal) ||
                        (Src2.AggregateVal[_i].FloatVal !=
                            Src2.AggregateVal[_i].FloatVal)));
        } else {
            for( size_t _i=0;_i<Src1.AggregateVal.size();_i++)
                Dest.AggregateVal[_i].IntVal = APInt(1,
                    ( (Src1.AggregateVal[_i].DoubleVal !=
                        Src1.AggregateVal[_i].DoubleVal) ||
                        (Src2.AggregateVal[_i].DoubleVal !=
                            Src2.AggregateVal[_i].DoubleVal)));
        }
    } else if (Ty->isFloatTy())
        Dest.IntVal = APInt(1,(Src1.FloatVal != Src1.FloatVal ||
            Src2.FloatVal != Src2.FloatVal));
    else {
        Dest.IntVal = APInt(1,(Src1.DoubleVal != Src1.DoubleVal ||
            Src2.DoubleVal != Src2.DoubleVal));
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFCMP_BOOL(GenericValue Src1, GenericValue Src2,
    const llvm::Type *Ty, const bool val) {
    TRACE_FUNC;
    GenericValue Dest;
    if(Ty->isVectorTy()) {
        ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());
        Dest.AggregateVal.resize( Src1.AggregateVal.size() );
        for( size_t _i=0; _i<Src1.AggregateVal.size(); _i++)
            Dest.AggregateVal[_i].IntVal = APInt(1,val);
    } else {
        Dest.IntVal = APInt(1, val);
    }

    return Dest;
}

void InstructionExecutor::visitFCmpInst(FCmpInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    llvm::Type *Ty    = I.getOperand(0)->getType();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue R;   // Result

    switch (I.getPredicate()) {
    default:
        borealis::dbgs() << "Don't know how to handle this FCmp predicate!\n-->" << I;
        UNREACHABLE(nullptr);
        break;
    case FCmpInst::FCMP_FALSE: R = ee->executeFCMP_BOOL(Src1, Src2, Ty, false);
    break;
    case FCmpInst::FCMP_TRUE:  R = ee->executeFCMP_BOOL(Src1, Src2, Ty, true);
    break;
    case FCmpInst::FCMP_ORD:   R = ee->executeFCMP_ORD(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_UNO:   R = ee->executeFCMP_UNO(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_UEQ:   R = ee->executeFCMP_UEQ(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_OEQ:   R = ee->executeFCMP_OEQ(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_UNE:   R = ee->executeFCMP_UNE(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_ONE:   R = ee->executeFCMP_ONE(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_ULT:   R = ee->executeFCMP_ULT(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_OLT:   R = ee->executeFCMP_OLT(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_UGT:   R = ee->executeFCMP_UGT(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_OGT:   R = ee->executeFCMP_OGT(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_ULE:   R = ee->executeFCMP_ULE(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_OLE:   R = ee->executeFCMP_OLE(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_UGE:   R = ee->executeFCMP_UGE(Src1, Src2, Ty); break;
    case FCmpInst::FCMP_OGE:   R = ee->executeFCMP_OGE(Src1, Src2, Ty); break;
    }

    SetValue(&I, R, SF);
}

GenericValue borealis::ExecutionEngine::executeCmpInst(unsigned predicate, GenericValue Src1,
        GenericValue Src2, llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Result;
    switch (predicate) {
    case ICmpInst::ICMP_EQ:    return executeICMP_EQ(Src1, Src2, Ty);
    case ICmpInst::ICMP_NE:    return executeICMP_NE(Src1, Src2, Ty);
    case ICmpInst::ICMP_UGT:   return executeICMP_UGT(Src1, Src2, Ty);
    case ICmpInst::ICMP_SGT:   return executeICMP_SGT(Src1, Src2, Ty);
    case ICmpInst::ICMP_ULT:   return executeICMP_ULT(Src1, Src2, Ty);
    case ICmpInst::ICMP_SLT:   return executeICMP_SLT(Src1, Src2, Ty);
    case ICmpInst::ICMP_UGE:   return executeICMP_UGE(Src1, Src2, Ty);
    case ICmpInst::ICMP_SGE:   return executeICMP_SGE(Src1, Src2, Ty);
    case ICmpInst::ICMP_ULE:   return executeICMP_ULE(Src1, Src2, Ty);
    case ICmpInst::ICMP_SLE:   return executeICMP_SLE(Src1, Src2, Ty);
    case FCmpInst::FCMP_ORD:   return executeFCMP_ORD(Src1, Src2, Ty);
    case FCmpInst::FCMP_UNO:   return executeFCMP_UNO(Src1, Src2, Ty);
    case FCmpInst::FCMP_OEQ:   return executeFCMP_OEQ(Src1, Src2, Ty);
    case FCmpInst::FCMP_UEQ:   return executeFCMP_UEQ(Src1, Src2, Ty);
    case FCmpInst::FCMP_ONE:   return executeFCMP_ONE(Src1, Src2, Ty);
    case FCmpInst::FCMP_UNE:   return executeFCMP_UNE(Src1, Src2, Ty);
    case FCmpInst::FCMP_OLT:   return executeFCMP_OLT(Src1, Src2, Ty);
    case FCmpInst::FCMP_ULT:   return executeFCMP_ULT(Src1, Src2, Ty);
    case FCmpInst::FCMP_OGT:   return executeFCMP_OGT(Src1, Src2, Ty);
    case FCmpInst::FCMP_UGT:   return executeFCMP_UGT(Src1, Src2, Ty);
    case FCmpInst::FCMP_OLE:   return executeFCMP_OLE(Src1, Src2, Ty);
    case FCmpInst::FCMP_ULE:   return executeFCMP_ULE(Src1, Src2, Ty);
    case FCmpInst::FCMP_OGE:   return executeFCMP_OGE(Src1, Src2, Ty);
    case FCmpInst::FCMP_UGE:   return executeFCMP_UGE(Src1, Src2, Ty);
    case FCmpInst::FCMP_FALSE: return executeFCMP_BOOL(Src1, Src2, Ty, false);
    case FCmpInst::FCMP_TRUE:  return executeFCMP_BOOL(Src1, Src2, Ty, true);
    default:
        borealis::dbgs() << "Unhandled Cmp predicate\n";
        UNREACHABLE(nullptr);
    }
}

llvm::GenericValue borealis::ExecutionEngine::executeBinary(
        llvm::Instruction::BinaryOps opcode,
        llvm::GenericValue Src1,
        llvm::GenericValue Src2,
        llvm::Type* Ty) {
    GenericValue R;

    // First process vector operation
    if (Ty->isVectorTy()) {
        ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());
        R.AggregateVal.resize(Src1.AggregateVal.size());

        // Macros to execute binary operation 'OP' over integer vectors
#define INTEGER_VECTOR_OPERATION(OP)                               \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)           \
    R.AggregateVal[i].IntVal =                                   \
    Src1.AggregateVal[i].IntVal OP Src2.AggregateVal[i].IntVal;

        // Additional macros to execute binary operations udiv/sdiv/urem/srem since
        // they have different notation.
#define INTEGER_VECTOR_FUNCTION(OP)                                \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)           \
    R.AggregateVal[i].IntVal =                                   \
    Src1.AggregateVal[i].IntVal.OP(Src2.AggregateVal[i].IntVal);

        // Macros to execute binary operation 'OP' over floating point llvm::Type TY
        // (float or double) vectors
#define FLOAT_VECTOR_FUNCTION(OP, TY)                               \
    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)          \
    R.AggregateVal[i].TY =                                      \
    Src1.AggregateVal[i].TY OP Src2.AggregateVal[i].TY;

        // Macros to choose appropriate TY: float or double and run operation
        // execution
#define FLOAT_VECTOR_OP(OP) {                                         \
    if (dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy())        \
    FLOAT_VECTOR_FUNCTION(OP, FloatVal)                               \
    else {                                                              \
        if (dyn_cast<VectorType>(Ty)->getElementType()->isDoubleTy())     \
        FLOAT_VECTOR_FUNCTION(OP, DoubleVal)                            \
        else {                                                            \
            borealis::dbgs() << "Unhandled llvm::Type for OP instruction: " << *Ty << "\n"; \
            UNREACHABLE(nullptr);                                            \
        }                                                                 \
    }                                                                   \
}

        switch(opcode){
        default:
            borealis::dbgs() << "Don't know how to handle this binary operator!";
            UNREACHABLE(nullptr);
            break;
        case Instruction::Add:   INTEGER_VECTOR_OPERATION(+) break;
        case Instruction::Sub:   INTEGER_VECTOR_OPERATION(-) break;
        case Instruction::Mul:   INTEGER_VECTOR_OPERATION(*) break;
        case Instruction::UDiv:  INTEGER_VECTOR_FUNCTION(udiv) break;
        case Instruction::SDiv:  INTEGER_VECTOR_FUNCTION(sdiv) break;
        case Instruction::URem:  INTEGER_VECTOR_FUNCTION(urem) break;
        case Instruction::SRem:  INTEGER_VECTOR_FUNCTION(srem) break;
        case Instruction::And:   INTEGER_VECTOR_OPERATION(&) break;
        case Instruction::Or:    INTEGER_VECTOR_OPERATION(|) break;
        case Instruction::Xor:   INTEGER_VECTOR_OPERATION(^) break;
        case Instruction::FAdd:  FLOAT_VECTOR_OP(+) break;
        case Instruction::FSub:  FLOAT_VECTOR_OP(-) break;
        case Instruction::FMul:  FLOAT_VECTOR_OP(*) break;
        case Instruction::FDiv:  FLOAT_VECTOR_OP(/) break;
        case Instruction::FRem:
            if (dyn_cast<VectorType>(Ty)->getElementType()->isFloatTy())
                for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
                    R.AggregateVal[i].FloatVal =
                        fmod(Src1.AggregateVal[i].FloatVal, Src2.AggregateVal[i].FloatVal);
            else {
                if (dyn_cast<VectorType>(Ty)->getElementType()->isDoubleTy())
                    for (unsigned i = 0; i < R.AggregateVal.size(); ++i)
                        R.AggregateVal[i].DoubleVal =
                            fmod(Src1.AggregateVal[i].DoubleVal, Src2.AggregateVal[i].DoubleVal);
                else {
                    borealis::dbgs() << "Unhandled llvm::Type for Rem instruction: " << *Ty << "\n";
                    UNREACHABLE(nullptr);
                }
            }
            break;
        }
    } else {
        switch (opcode) {
        default:
            borealis::dbgs() << "Don't know how to handle this binary operator!";
            UNREACHABLE(nullptr);
            break;
        case Instruction::Add:   R.IntVal = Src1.IntVal + Src2.IntVal; break;
        case Instruction::Sub:   R.IntVal = Src1.IntVal - Src2.IntVal; break;
        case Instruction::Mul:   R.IntVal = Src1.IntVal * Src2.IntVal; break;
        case Instruction::FAdd:  executeFAddInst(R, Src1, Src2, Ty); break;
        case Instruction::FSub:  executeFSubInst(R, Src1, Src2, Ty); break;
        case Instruction::FMul:  executeFMulInst(R, Src1, Src2, Ty); break;
        case Instruction::FDiv:  executeFDivInst(R, Src1, Src2, Ty); break;
        case Instruction::FRem:  executeFRemInst(R, Src1, Src2, Ty); break;
        case Instruction::UDiv:  R.IntVal = Src1.IntVal.udiv(Src2.IntVal); break;
        case Instruction::SDiv:  R.IntVal = Src1.IntVal.sdiv(Src2.IntVal); break;
        case Instruction::URem:  R.IntVal = Src1.IntVal.urem(Src2.IntVal); break;
        case Instruction::SRem:  R.IntVal = Src1.IntVal.srem(Src2.IntVal); break;
        case Instruction::And:   R.IntVal = Src1.IntVal & Src2.IntVal; break;
        case Instruction::Or:    R.IntVal = Src1.IntVal | Src2.IntVal; break;
        case Instruction::Xor:   R.IntVal = Src1.IntVal ^ Src2.IntVal; break;
        }
    }

    return R;
}

void InstructionExecutor::visitBinaryOperator(BinaryOperator &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    llvm::Type *Ty    = I.getOperand(0)->getType();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue R = ee->executeBinary(I.getOpcode(), Src1, Src2, Ty);

    SetValue(&I, R, SF);
}

GenericValue borealis::ExecutionEngine::executeSelectInst(GenericValue Src1, GenericValue Src2,
    GenericValue Src3, const llvm::Type *Ty) {
    TRACE_FUNC;
    GenericValue Dest;
    if(Ty->isVectorTy()) {
        ASSERTC(Src1.AggregateVal.size() == Src2.AggregateVal.size());
        ASSERTC(Src2.AggregateVal.size() == Src3.AggregateVal.size());
        Dest.AggregateVal.resize( Src1.AggregateVal.size() );
        for (size_t i = 0; i < Src1.AggregateVal.size(); ++i)
            Dest.AggregateVal[i] = (Src1.AggregateVal[i].IntVal == 0) ?
                Src3.AggregateVal[i] : Src2.AggregateVal[i];
    } else {
        Dest = (Src1.IntVal == 0) ? Src3 : Src2;
    }
    return Dest;
}

void InstructionExecutor::visitSelectInst(SelectInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    const llvm::Type * Ty = I.getOperand(0)->getType();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Src3 = ee->getOperandValue(I.getOperand(2), SF);
    GenericValue R = ee->executeSelectInst(Src1, Src2, Src3, Ty);
    SetValue(&I, R, SF);
}

//===----------------------------------------------------------------------===//
//                     Terminator Instruction Implementations
//===----------------------------------------------------------------------===//

void borealis::ExecutionEngine::exitCalled(GenericValue GV) {
    TRACE_FUNC;
    // runAtExitHandlers() assumes there are no stack frames, but
    // if exit() was called, then it had a stack frame. Blow away
    // the stack before interpreting atexit handlers.
    ECStack.clear();
    runAtExitHandlers();
    // TODO: implement exit()

    TRACES() << "exit() called with code" << util::toString(GV.IntVal) << endl;
}

/// Pop the last stack frame off of ECStack and then copy the result
/// back into the result variable if we are not returning void. The
/// result variable may be the ExitValue, or the Value of the calling
/// CallInst if there was a previous stack frame. This method may
/// invalidate any ECStack iterators you have. This method also takes
/// care of switching to the normal destination BB, if we are returning
/// from an invoke.
///
void borealis::ExecutionEngine::popStackAndReturnValueToCaller(llvm::Type *RetTy,
    GenericValue Result) {
    TRACE_FUNC;
    // Pop the current stack frame.
    ECStack.pop_back();

    if (ECStack.empty()) {  // Finished main.  Put result into exit code...
        if (RetTy && !RetTy->isVoidTy()) {          // Nonvoid return llvm::Type?
            ExitValue = Result;   // Capture the exit value of the program
        } else {
            memset(&ExitValue.Untyped, 0, sizeof(ExitValue.Untyped));
        }
    } else {
        // If we have a previous stack frame, and we have a previous call,
        // fill in the return value...
        ExecutorContext &CallingSF = ECStack.back();
        if (Instruction *I = CallingSF.Caller.getInstruction()) {
            // Save result...
            if (RetTy && !CallingSF.Caller.getType()->isVoidTy())
                SetValue(I, Result, CallingSF);
            if (InvokeInst *II = dyn_cast<InvokeInst> (I))
                SwitchToNewBasicBlock (II->getNormalDest (), CallingSF);
            CallingSF.Caller = CallSite();          // We returned from the call...
        }
    }
}

void InstructionExecutor::visitReturnInst(ReturnInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    llvm::Type *RetTy = llvm::Type::getVoidTy(I.getContext());
    GenericValue Result;

    // Save away the return value... (if we are not 'ret void')
    if (I.getNumOperands()) {
        RetTy  = I.getReturnValue()->getType();
        Result = ee->getOperandValue(I.getReturnValue(), SF);
    }

    ee->popStackAndReturnValueToCaller(RetTy, Result);
}

void InstructionExecutor::visitUnreachableInst(UnreachableInst&) {
    TRACE_FUNC;
    throw unreachable_reached{};
}

void InstructionExecutor::visitBranchInst(BranchInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    BasicBlock *Dest;

    Dest = I.getSuccessor(0);          // Uncond branches have a fixed dest...
    if (!I.isUnconditional()) {
        Value *Cond = I.getCondition();
        if (ee->getOperandValue(Cond, SF).IntVal == 0) // If false cond...
            Dest = I.getSuccessor(1);
    }
    ee->SwitchToNewBasicBlock(Dest, SF);
}

void InstructionExecutor::visitSwitchInst(SwitchInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    Value* Cond = I.getCondition();
    llvm::Type *ElTy = Cond->getType();
    GenericValue CondVal = ee->getOperandValue(Cond, SF);

    // Check to see if any of the cases match...
    BasicBlock *Dest = nullptr;
    for (SwitchInst::CaseIt i = I.case_begin(), e = I.case_end(); i != e; ++i) {
        GenericValue CaseVal = ee->getOperandValue(i.getCaseValue(), SF);
        if (ee->executeICMP_EQ(CondVal, CaseVal, ElTy).IntVal != 0) {
            Dest = cast<BasicBlock>(i.getCaseSuccessor());
            break;
        }
    }
    if (!Dest) Dest = I.getDefaultDest();   // No cases matched: use default
    ee->SwitchToNewBasicBlock(Dest, SF);
}

void InstructionExecutor::visitIndirectBrInst(IndirectBrInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    void *Dest = GVTOP(ee->getOperandValue(I.getAddress(), SF));
    ee->SwitchToNewBasicBlock((BasicBlock*)Dest, SF);
}


// SwitchToNewBasicBlock - This method is used to jump to a new basic block.
// This function handles the actual updating of block and instruction iterators
// as well as execution of all of the PHI nodes in the destination block.
//
// This method does this because all of the PHI nodes must be executed
// atomically, reading their inputs before any of the results are updated.  Not
// doing this can cause problems if the PHI nodes depend on other PHI nodes for
// their inputs.  If the input PHI node is updated before it is read, incorrect
// results can happen.  Thus we use a two phase approach.
//
void borealis::ExecutionEngine::SwitchToNewBasicBlock(BasicBlock *Dest, ExecutorContext &SF){
    TRACE_FUNC;
    BasicBlock *PrevBB = SF.CurBB;      // Remember where we came from...
    SF.CurBB   = Dest;                  // Update CurBB to branch destination
    SF.CurInst = SF.CurBB->begin();     // Update new instruction ptr...

    if (!isa<PHINode>(SF.CurInst)) return;  // Nothing fancy to do

    // Loop over all of the PHI nodes in the current block, reading their inputs.
    std::vector<GenericValue> ResultValues;

    for (; PHINode *PN = dyn_cast<PHINode>(SF.CurInst); ++SF.CurInst) {
        // Search for the value corresponding to this previous bb...
        int i = PN->getBasicBlockIndex(PrevBB);
        ASSERTC(i != -1 && "PHINode doesn't contain entry for predecessor??");
        Value *IncomingValue = PN->getIncomingValue(i);

        // Save the incoming value for this PHI node...
        ResultValues.push_back(getOperandValue(IncomingValue, SF));
    }

    // Now loop over all of the PHI nodes setting their values...
    SF.CurInst = SF.CurBB->begin();
    for (unsigned i = 0; isa<PHINode>(SF.CurInst); ++SF.CurInst, ++i) {
        PHINode *PN = cast<PHINode>(SF.CurInst);
        SetValue(PN, ResultValues[i], SF);
    }
}

//===----------------------------------------------------------------------===//
//                     Memory Instruction Implementations
//===----------------------------------------------------------------------===//

GenericValue borealis::ExecutionEngine::executeAlloca(size_t size) {
    TRACE_FUNC;
    // TODO

    // Allocate enough memory to hold the llvm::Type...
    void *Memory = Mem.AllocateMemory(size);
    return PTOGV(Memory);
}

void InstructionExecutor::visitAllocaInst(AllocaInst &I) {
    TRACE_FUNC;
    // TODO

    ExecutorContext &SF = ee->getCurrentContext();

    llvm::Type *Ty = I.getType()->getElementType();  // llvm::Type to be allocated

    // Get the number of elements being allocated by the array...
    unsigned NumElements =
        ee->getOperandValue(I.getOperand(0), SF).IntVal.getZExtValue();

    unsigned TypeSize = (size_t)ee->getDataLayout()->getTypeAllocSize(Ty);

    // Avoid malloc-ing zero bytes, use max()...
    unsigned MemToAlloc = std::max(1U, NumElements * TypeSize);

    GenericValue Result = ee->executeAlloca(MemToAlloc);
    ASSERTC(Result.PointerVal && "Null pointer returned by malloc!");
    SetValue(&I, Result, SF);

}

// getElementOffset - The workhorse for getelementptr.
//
GenericValue borealis::ExecutionEngine::executeGEPOperation(Value *Ptr, gep_type_iterator I,
    gep_type_iterator E,
    ExecutorContext &SF) {
    TRACE_FUNC;
    ASSERT(Ptr->getType()->isPointerTy(),
        "Cannot getElementOffset of a nonpointer llvm::Type!");

    uint64_t Total = 0;

    for (; I != E; ++I) {
        if (StructType *STy = dyn_cast<StructType>(*I)) {
            const StructLayout *SLO = TD->getStructLayout(STy);

            const ConstantInt *CPU = cast<ConstantInt>(I.getOperand());
            unsigned Index = unsigned(CPU->getZExtValue());

            Total += SLO->getElementOffset(Index);
        } else {
            SequentialType *ST = cast<SequentialType>(*I);
            // Get the index number for the array... which must be long llvm::Type...
            GenericValue IdxGV = getOperandValue(I.getOperand(), SF);

            int64_t Idx;
            unsigned BitWidth =
                cast<IntegerType>(I.getOperand()->getType())->getBitWidth();
            if (BitWidth == 32)
                Idx = (int64_t)(int32_t)IdxGV.IntVal.getZExtValue();
            else {
                ASSERT(BitWidth == 64, "Invalid index llvm::Type for getelementptr");
                Idx = (int64_t)IdxGV.IntVal.getZExtValue();
            }
            Total += TD->getTypeAllocSize(ST->getElementType())*Idx;
        }
    }

    GenericValue Result;
    Result.PointerVal = ((char*)getOperandValue(Ptr, SF).PointerVal) + Total;
    return Result;
}

void InstructionExecutor::visitGetElementPtrInst(GetElementPtrInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeGEPOperation(I.getPointerOperand(),
        gep_type_begin(I), gep_type_end(I), SF), SF);
}

void InstructionExecutor::visitLoadInst(LoadInst &I) {
    TRACE_FUNC;
    // TODO
    ExecutorContext &SF = ee->getCurrentContext();
    GenericValue gsrc = ee->getOperandValue(I.getPointerOperand(), SF);
    void* src = GVTOP(gsrc);

    if(ee->isSymbolicPointer(src)) return;

    GenericValue Result;
    ee->LoadValueFromMemory(Result, static_cast<const byte*>(src), I.getType());
    SetValue(&I, Result, SF);
}

void InstructionExecutor::visitStoreInst(StoreInst &I) {
    TRACE_FUNC;
    // TODO

    ExecutorContext &SF = ee->getCurrentContext();
    GenericValue Val = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue gsrc = ee->getOperandValue(I.getPointerOperand(), SF);
    void* src = GVTOP(gsrc);

    if(ee->isSymbolicPointer(src)) throw std::logic_error("Cannot store to a symbolic location"); // FIXME

    ee->StoreValueToMemory(Val, static_cast<byte*>(src), I.getOperand(0)->getType());
}

//===----------------------------------------------------------------------===//
//                 Miscellaneous Instruction Implementations
//===----------------------------------------------------------------------===//

void borealis::ExecutionEngine::executeCall(llvm::CallSite CS) {
    TRACE_FUNC;

    ExecutorContext &SF = getCurrentContext();

    // Check to see if this is an intrinsic function call...
    Function *F = CS.getCalledFunction();

    if(F && F->isDeclaration()) {
        switch(IM->getIntrinsicType(F)) {
        case function_type::UNKNOWN:
        case function_type::INTRINSIC_MALLOC:
        case function_type::INTRINSIC_ALLOC:
        case function_type::BUILTIN_BOR_ASSERT:
        case function_type::BUILTIN_BOR_ASSUME:
            break;
        case function_type::INTRINSIC_ANNOTATION: {
            auto Anno =
                static_cast<Annotation*>(MDNode2Ptr(CS.getInstruction()->getMetadata("anno.ptr")));
            TRACE_FMT("%d", Anno);
            ASSERTC(Anno);
            auto sharedAnno = materialize(Anno->shared_from_this(), FN, VIT);

            AE.transform(sharedAnno);
            return;
        }
        case function_type::ACTION_DEFECT: {
            throw assertion_failed{};
        }
        default: // FIXME: process annotation-related intrinsics when we start checking annotations
            return;
        }
    }



    SF.Caller = CS;
    std::vector<GenericValue> ArgVals;
    const unsigned NumArgs = SF.Caller.arg_size();
    ArgVals.reserve(NumArgs);
    uint16_t pNum = 1;
    for (CallSite::arg_iterator i = SF.Caller.arg_begin(),
        e = SF.Caller.arg_end(); i != e; ++i, ++pNum) {
        Value *V = *i;
        ArgVals.push_back(getOperandValue(V, SF));
    }

    if (F && F->isDeclaration())
        switch (F->getIntrinsicID()) {
        default: return;
        case Intrinsic::not_intrinsic:
            break;
        case Intrinsic::vastart: { // va_start
            GenericValue ArgIndex;
            ArgIndex.UIntPairVal.first = ECStack.size() - 1;
            ArgIndex.UIntPairVal.second = 0;
            SetValue(CS.getInstruction(), ArgIndex, SF);
            return;
        }
        case Intrinsic::vaend:    // va_end is a noop for the interpreter
        case Intrinsic::dbg_declare:
        case Intrinsic::dbg_value:
            return;
        case Intrinsic::vacopy:   // va_copy: dest = src
            SetValue(CS.getInstruction(), getOperandValue(*CS.arg_begin(), SF), SF);
            return;
        case Intrinsic::memset:
        case Intrinsic::memcpy:
        case Intrinsic::memmove:
            break;
        }

    if(!F) {
        GenericValue SRC = getOperandValue(SF.Caller.getCalledValue(), SF);
        callFunction(Mem.accessFunction(SRC.PointerVal), ArgVals);
    } else {
        callFunction(F, ArgVals);
    }
}

void InstructionExecutor::visitCallSite(CallSite CS) {
    TRACE_FUNC;

    ee->executeCall(CS);
}

// auxiliary function for shift operations
static unsigned getShiftAmount(uint64_t orgShiftAmount,
    llvm::APInt valueToShift) {
    TRACE_FUNC;
    unsigned valueWidth = valueToShift.getBitWidth();
    if (orgShiftAmount < (uint64_t)valueWidth)
        return orgShiftAmount;
    // according to the llvm documentation, if orgShiftAmount > valueWidth,
    // the result is undfeined. but we do shift by this rule:
    return (NextPowerOf2(valueWidth-1) - 1) & orgShiftAmount;
}


void InstructionExecutor::visitShl(BinaryOperator &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Dest;
    const llvm::Type *Ty = I.getType();

    if (Ty->isVectorTy()) {
        uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
        ASSERTC(src1Size == Src2.AggregateVal.size());
        for (unsigned i = 0; i < src1Size; i++) {
            GenericValue Result;
            uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
            llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
            Result.IntVal = valueToShift.shl(getShiftAmount(shiftAmount, valueToShift));
            Dest.AggregateVal.push_back(Result);
        }
    } else {
        // scalar
        uint64_t shiftAmount = Src2.IntVal.getZExtValue();
        llvm::APInt valueToShift = Src1.IntVal;
        Dest.IntVal = valueToShift.shl(getShiftAmount(shiftAmount, valueToShift));
    }

    SetValue(&I, Dest, SF);
}

void InstructionExecutor::visitLShr(BinaryOperator &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Dest;
    const llvm::Type *Ty = I.getType();

    if (Ty->isVectorTy()) {
        uint32_t src1Size = uint32_t(Src1.AggregateVal.size());
        ASSERTC(src1Size == Src2.AggregateVal.size());
        for (unsigned i = 0; i < src1Size; i++) {
            GenericValue Result;
            uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
            llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
            Result.IntVal = valueToShift.lshr(getShiftAmount(shiftAmount, valueToShift));
            Dest.AggregateVal.push_back(Result);
        }
    } else {
        // scalar
        uint64_t shiftAmount = Src2.IntVal.getZExtValue();
        llvm::APInt valueToShift = Src1.IntVal;
        Dest.IntVal = valueToShift.lshr(getShiftAmount(shiftAmount, valueToShift));
    }

    SetValue(&I, Dest, SF);
}

void InstructionExecutor::visitAShr(BinaryOperator &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Dest;
    const llvm::Type *Ty = I.getType();

    if (Ty->isVectorTy()) {
        size_t src1Size = Src1.AggregateVal.size();
        ASSERTC(src1Size == Src2.AggregateVal.size());
        for (unsigned i = 0; i < src1Size; i++) {
            GenericValue Result;
            uint64_t shiftAmount = Src2.AggregateVal[i].IntVal.getZExtValue();
            llvm::APInt valueToShift = Src1.AggregateVal[i].IntVal;
            Result.IntVal = valueToShift.ashr(getShiftAmount(shiftAmount, valueToShift));
            Dest.AggregateVal.push_back(Result);
        }
    } else {
        // scalar
        uint64_t shiftAmount = Src2.IntVal.getZExtValue();
        llvm::APInt valueToShift = Src1.IntVal;
        Dest.IntVal = valueToShift.ashr(getShiftAmount(shiftAmount, valueToShift));
    }

    SetValue(&I, Dest, SF);
}

GenericValue borealis::ExecutionEngine::executeTruncInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);
    llvm::Type *SrcTy = SrcVal->getType();
    if (SrcTy->isVectorTy()) {
        llvm::Type *DstVecTy = DstTy->getScalarType();
        unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
        unsigned NumElts = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal
        Dest.AggregateVal.resize(NumElts);
        for (unsigned i = 0; i < NumElts; i++)
            Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.trunc(DBitWidth);
    } else {
        IntegerType *DITy = cast<IntegerType>(DstTy);
        unsigned DBitWidth = DITy->getBitWidth();
        Dest.IntVal = Src.IntVal.trunc(DBitWidth);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeSExtInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    const llvm::Type *SrcTy = SrcVal->getType();
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);
    if (SrcTy->isVectorTy()) {
        const llvm::Type *DstVecTy = DstTy->getScalarType();
        unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal.
        Dest.AggregateVal.resize(size);
        for (unsigned i = 0; i < size; i++)
            Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.sext(DBitWidth);
    } else {
        const IntegerType *DITy = cast<IntegerType>(DstTy);
        unsigned DBitWidth = DITy->getBitWidth();
        Dest.IntVal = Src.IntVal.sext(DBitWidth);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeZExtInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    const llvm::Type *SrcTy = SrcVal->getType();
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);
    if (SrcTy->isVectorTy()) {
        const llvm::Type *DstVecTy = DstTy->getScalarType();
        unsigned DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();

        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal.
        Dest.AggregateVal.resize(size);
        for (unsigned i = 0; i < size; i++)
            Dest.AggregateVal[i].IntVal = Src.AggregateVal[i].IntVal.zext(DBitWidth);
    } else {
        const IntegerType *DITy = cast<IntegerType>(DstTy);
        unsigned DBitWidth = DITy->getBitWidth();
        Dest.IntVal = Src.IntVal.zext(DBitWidth);
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFPTruncInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
        ASSERTC(SrcVal->getType()->getScalarType()->isDoubleTy() &&
            DstTy->getScalarType()->isFloatTy() &&
            "Invalid FPTrunc instruction");

        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal.
        Dest.AggregateVal.resize(size);
        for (unsigned i = 0; i < size; i++)
            Dest.AggregateVal[i].FloatVal = (float)Src.AggregateVal[i].DoubleVal;
    } else {
        ASSERT(SrcVal->getType()->isDoubleTy() && DstTy->isFloatTy(),
            "Invalid FPTrunc instruction");
        Dest.FloatVal = (float)Src.DoubleVal;
    }

    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFPExtInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
        ASSERT(SrcVal->getType()->getScalarType()->isFloatTy() &&
            DstTy->getScalarType()->isDoubleTy(), "Invalid FPExt instruction");

        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal.
        Dest.AggregateVal.resize(size);
        for (unsigned i = 0; i < size; i++)
            Dest.AggregateVal[i].DoubleVal = (double)Src.AggregateVal[i].FloatVal;
    } else {
        ASSERTC(SrcVal->getType()->isFloatTy() && DstTy->isDoubleTy() &&
            "Invalid FPExt instruction");
        Dest.DoubleVal = (double)Src.FloatVal;
    }

    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFPToUIInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    llvm::Type *SrcTy = SrcVal->getType();
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if (SrcTy->getTypeID() == llvm::Type::VectorTyID) {
        const llvm::Type *DstVecTy = DstTy->getScalarType();
        const llvm::Type *SrcVecTy = SrcTy->getScalarType();
        uint32_t DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal.
        Dest.AggregateVal.resize(size);

        if (SrcVecTy->getTypeID() == llvm::Type::FloatTyID) {
            ASSERTC(SrcVecTy->isFloatingPointTy() && "Invalid FPToUI instruction");
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].IntVal = APIntOps::RoundFloatToAPInt(
                    Src.AggregateVal[i].FloatVal, DBitWidth);
        } else {
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].IntVal = APIntOps::RoundDoubleToAPInt(
                    Src.AggregateVal[i].DoubleVal, DBitWidth);
        }
    } else {
        // scalar
        uint32_t DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
        ASSERTC(SrcTy->isFloatingPointTy() && "Invalid FPToUI instruction");

        if (SrcTy->getTypeID() == llvm::Type::FloatTyID)
            Dest.IntVal = APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
        else {
            Dest.IntVal = APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
        }
    }

    return Dest;
}

GenericValue borealis::ExecutionEngine::executeFPToSIInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    llvm::Type *SrcTy = SrcVal->getType();
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if (SrcTy->getTypeID() == llvm::Type::VectorTyID) {
        const llvm::Type *DstVecTy = DstTy->getScalarType();
        const llvm::Type *SrcVecTy = SrcTy->getScalarType();
        uint32_t DBitWidth = cast<IntegerType>(DstVecTy)->getBitWidth();
        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal
        Dest.AggregateVal.resize(size);

        if (SrcVecTy->getTypeID() == llvm::Type::FloatTyID) {
            ASSERTC(SrcVecTy->isFloatingPointTy() && "Invalid FPToSI instruction");
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].IntVal = APIntOps::RoundFloatToAPInt(
                    Src.AggregateVal[i].FloatVal, DBitWidth);
        } else {
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].IntVal = APIntOps::RoundDoubleToAPInt(
                    Src.AggregateVal[i].DoubleVal, DBitWidth);
        }
    } else {
        // scalar
        unsigned DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
        ASSERTC(SrcTy->isFloatingPointTy() && "Invalid FPToSI instruction");

        if (SrcTy->getTypeID() == llvm::Type::FloatTyID)
            Dest.IntVal = APIntOps::RoundFloatToAPInt(Src.FloatVal, DBitWidth);
        else {
            Dest.IntVal = APIntOps::RoundDoubleToAPInt(Src.DoubleVal, DBitWidth);
        }
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeUIToFPInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
        const llvm::Type *DstVecTy = DstTy->getScalarType();
        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal
        Dest.AggregateVal.resize(size);

        if (DstVecTy->getTypeID() == llvm::Type::FloatTyID) {
            ASSERTC(DstVecTy->isFloatingPointTy() && "Invalid UIToFP instruction");
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].FloatVal =
                    APIntOps::RoundAPIntToFloat(Src.AggregateVal[i].IntVal);
        } else {
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].DoubleVal =
                    APIntOps::RoundAPIntToDouble(Src.AggregateVal[i].IntVal);
        }
    } else {
        // scalar
        ASSERTC(DstTy->isFloatingPointTy() && "Invalid UIToFP instruction");
        if (DstTy->getTypeID() == llvm::Type::FloatTyID)
            Dest.FloatVal = APIntOps::RoundAPIntToFloat(Src.IntVal);
        else {
            Dest.DoubleVal = APIntOps::RoundAPIntToDouble(Src.IntVal);
        }
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeSIToFPInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if (SrcVal->getType()->getTypeID() == llvm::Type::VectorTyID) {
        const llvm::Type *DstVecTy = DstTy->getScalarType();
        unsigned size = Src.AggregateVal.size();
        // the sizes of src and dst vectors must be equal
        Dest.AggregateVal.resize(size);

        if (DstVecTy->getTypeID() == llvm::Type::FloatTyID) {
            ASSERTC(DstVecTy->isFloatingPointTy() && "Invalid SIToFP instruction");
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].FloatVal =
                    APIntOps::RoundSignedAPIntToFloat(Src.AggregateVal[i].IntVal);
        } else {
            for (unsigned i = 0; i < size; i++)
                Dest.AggregateVal[i].DoubleVal =
                    APIntOps::RoundSignedAPIntToDouble(Src.AggregateVal[i].IntVal);
        }
    } else {
        // scalar
        ASSERTC(DstTy->isFloatingPointTy() && "Invalid SIToFP instruction");

        if (DstTy->getTypeID() == llvm::Type::FloatTyID)
            Dest.FloatVal = APIntOps::RoundSignedAPIntToFloat(Src.IntVal);
        else {
            Dest.DoubleVal = APIntOps::RoundSignedAPIntToDouble(Src.IntVal);
        }
    }

    return Dest;
}

GenericValue borealis::ExecutionEngine::executePtrToIntInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    uint32_t DBitWidth = cast<IntegerType>(DstTy)->getBitWidth();
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);
    ASSERTC(SrcVal->getType()->isPointerTy() && "Invalid PtrToInt instruction");

    Dest.IntVal = APInt(DBitWidth, (intptr_t) Src.PointerVal);
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeIntToPtrInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {
    TRACE_FUNC;
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);
    ASSERTC(DstTy->isPointerTy() && "Invalid PtrToInt instruction");

    uint32_t PtrSize = TD->getPointerSizeInBits();
    if (PtrSize != Src.IntVal.getBitWidth())
        Src.IntVal = Src.IntVal.zextOrTrunc(PtrSize);

    Dest.PointerVal = PointerTy(intptr_t(Src.IntVal.getZExtValue()));
    return Dest;
}

GenericValue borealis::ExecutionEngine::executeBitCastInst(Value *SrcVal, llvm::Type *DstTy,
    ExecutorContext &SF) {

   TRACE_FUNC;
    // This instruction supports bitwise conversion of vectors to integers and
    // to vectors of other Types (as long as they have the same size)
    llvm::Type *SrcTy = SrcVal->getType();
    GenericValue Dest, Src = getOperandValue(SrcVal, SF);

    if ((SrcTy->getTypeID() == llvm::Type::VectorTyID) ||
        (DstTy->getTypeID() == llvm::Type::VectorTyID)) {
        // vector src bitcast to vector dst or vector src bitcast to scalar dst or
        // scalar src bitcast to vector dst
        bool isLittleEndian = TD->isLittleEndian();
        GenericValue TempDst, TempSrc, SrcVec;
        const llvm::Type *SrcElemTy;
        const llvm::Type *DstElemTy;
        unsigned SrcBitSize;
        unsigned DstBitSize;
        unsigned SrcNum;
        unsigned DstNum;

        if (SrcTy->getTypeID() == llvm::Type::VectorTyID) {
            SrcElemTy = SrcTy->getScalarType();
            SrcBitSize = SrcTy->getScalarSizeInBits();
            SrcNum = Src.AggregateVal.size();
            SrcVec = Src;
        } else {
            // if src is scalar value, make it vector <1 x llvm::Type>
            SrcElemTy = SrcTy;
            SrcBitSize = SrcTy->getPrimitiveSizeInBits();
            SrcNum = 1;
            SrcVec.AggregateVal.push_back(Src);
        }

        if (DstTy->getTypeID() == llvm::Type::VectorTyID) {
            DstElemTy = DstTy->getScalarType();
            DstBitSize = DstTy->getScalarSizeInBits();
            DstNum = (SrcNum * SrcBitSize) / DstBitSize;
        } else {
            DstElemTy = DstTy;
            DstBitSize = DstTy->getPrimitiveSizeInBits();
            DstNum = 1;
        }

        if (SrcNum * SrcBitSize != DstNum * DstBitSize)
            UNREACHABLE("Invalid BitCast");

        // If src is floating point, cast to integer first.
        TempSrc.AggregateVal.resize(SrcNum);
        if (SrcElemTy->isFloatTy()) {
            for (unsigned i = 0; i < SrcNum; i++)
                TempSrc.AggregateVal[i].IntVal =
                    APInt::floatToBits(SrcVec.AggregateVal[i].FloatVal);

        } else if (SrcElemTy->isDoubleTy()) {
            for (unsigned i = 0; i < SrcNum; i++)
                TempSrc.AggregateVal[i].IntVal =
                    APInt::doubleToBits(SrcVec.AggregateVal[i].DoubleVal);
        } else if (SrcElemTy->isIntegerTy()) {
            for (unsigned i = 0; i < SrcNum; i++)
                TempSrc.AggregateVal[i].IntVal = SrcVec.AggregateVal[i].IntVal;
        } else {
            // Pointers are not allowed as the element llvm::Type of vector.
            UNREACHABLE("Invalid Bitcast");
        }

        // now TempSrc is integer llvm::Type vector
        if (DstNum < SrcNum) {
            // Example: bitcast <4 x i32> <i32 0, i32 1, i32 2, i32 3> to <2 x i64>
            unsigned Ratio = SrcNum / DstNum;
            unsigned SrcElt = 0;
            for (unsigned i = 0; i < DstNum; i++) {
                GenericValue Elt;
                Elt.IntVal = 0;
                Elt.IntVal = Elt.IntVal.zext(DstBitSize);
                unsigned ShiftAmt = isLittleEndian ? 0 : SrcBitSize * (Ratio - 1);
                for (unsigned j = 0; j < Ratio; j++) {
                    APInt Tmp;
                    Tmp = Tmp.zext(SrcBitSize);
                    Tmp = TempSrc.AggregateVal[SrcElt++].IntVal;
                    Tmp = Tmp.zext(DstBitSize);
                    Tmp = Tmp.shl(ShiftAmt);
                    ShiftAmt += isLittleEndian ? SrcBitSize : -SrcBitSize;
                    Elt.IntVal |= Tmp;
                }
                TempDst.AggregateVal.push_back(Elt);
            }
        } else {
            // Example: bitcast <2 x i64> <i64 0, i64 1> to <4 x i32>
            unsigned Ratio = DstNum / SrcNum;
            for (unsigned i = 0; i < SrcNum; i++) {
                unsigned ShiftAmt = isLittleEndian ? 0 : DstBitSize * (Ratio - 1);
                for (unsigned j = 0; j < Ratio; j++) {
                    GenericValue Elt;
                    Elt.IntVal = Elt.IntVal.zext(SrcBitSize);
                    Elt.IntVal = TempSrc.AggregateVal[i].IntVal;
                    Elt.IntVal = Elt.IntVal.lshr(ShiftAmt);
                    // it could be DstBitSize == SrcBitSize, so check it
                    if (DstBitSize < SrcBitSize)
                        Elt.IntVal = Elt.IntVal.trunc(DstBitSize);
                    ShiftAmt += isLittleEndian ? DstBitSize : -DstBitSize;
                    TempDst.AggregateVal.push_back(Elt);
                }
            }
        }

        // convert result from integer to specified llvm::Type
        if (DstTy->getTypeID() == llvm::Type::VectorTyID) {
            if (DstElemTy->isDoubleTy()) {
                Dest.AggregateVal.resize(DstNum);
                for (unsigned i = 0; i < DstNum; i++)
                    Dest.AggregateVal[i].DoubleVal =
                        TempDst.AggregateVal[i].IntVal.bitsToDouble();
            } else if (DstElemTy->isFloatTy()) {
                Dest.AggregateVal.resize(DstNum);
                for (unsigned i = 0; i < DstNum; i++)
                    Dest.AggregateVal[i].FloatVal =
                        TempDst.AggregateVal[i].IntVal.bitsToFloat();
            } else {
                Dest = TempDst;
            }
        } else {
            if (DstElemTy->isDoubleTy())
                Dest.DoubleVal = TempDst.AggregateVal[0].IntVal.bitsToDouble();
            else if (DstElemTy->isFloatTy()) {
                Dest.FloatVal = TempDst.AggregateVal[0].IntVal.bitsToFloat();
            } else {
                Dest.IntVal = TempDst.AggregateVal[0].IntVal;
            }
        }
    } else { //  if ((SrcTy->getTypeID() == llvm::Type::VectorTyID) ||
        //     (DstTy->getTypeID() == llvm::Type::VectorTyID))

        // scalar src bitcast to scalar dst
        if (DstTy->isPointerTy()) {
            ASSERTC(SrcTy->isPointerTy() && "Invalid BitCast");
            Dest.PointerVal = Src.PointerVal;
        } else if (DstTy->isIntegerTy()) {
            if (SrcTy->isFloatTy())
                Dest.IntVal = APInt::floatToBits(Src.FloatVal);
            else if (SrcTy->isDoubleTy()) {
                Dest.IntVal = APInt::doubleToBits(Src.DoubleVal);
            } else if (SrcTy->isIntegerTy()) {
                Dest.IntVal = Src.IntVal;
            } else {
                UNREACHABLE("Invalid BitCast");
            }
        } else if (DstTy->isFloatTy()) {
            if (SrcTy->isIntegerTy())
                Dest.FloatVal = Src.IntVal.bitsToFloat();
            else {
                Dest.FloatVal = Src.FloatVal;
            }
        } else if (DstTy->isDoubleTy()) {
            if (SrcTy->isIntegerTy())
                Dest.DoubleVal = Src.IntVal.bitsToDouble();
            else {
                Dest.DoubleVal = Src.DoubleVal;
            }
        } else {
            UNREACHABLE("Invalid Bitcast");
        }
    }

    return Dest;
}

void InstructionExecutor::visitTruncInst(TruncInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitSExtInst(SExtInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeSExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitZExtInst(ZExtInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeZExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitFPTruncInst(FPTruncInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeFPTruncInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitFPExtInst(FPExtInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeFPExtInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitUIToFPInst(UIToFPInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeUIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitSIToFPInst(SIToFPInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeSIToFPInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitFPToUIInst(FPToUIInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeFPToUIInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitFPToSIInst(FPToSIInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeFPToSIInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitPtrToIntInst(PtrToIntInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executePtrToIntInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitIntToPtrInst(IntToPtrInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeIntToPtrInst(I.getOperand(0), I.getType(), SF), SF);
}

void InstructionExecutor::visitBitCastInst(BitCastInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    SetValue(&I, ee->executeBitCastInst(I.getOperand(0), I.getType(), SF), SF);
}

#define IMPLEMENT_VAARG(TY) \
    case llvm::Type::TY##TyID: Dest.TY##Val = Src.TY##Val; break

void InstructionExecutor::visitVAArgInst(VAArgInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();

    // Get the incoming valist parameter.  LLI treats the valist as a
    // (ec-stack-depth var-arg-index) pair.
    GenericValue VAList = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Dest;
    GenericValue Src = ee->getContext(VAList.UIntPairVal.first).VarArgs[VAList.UIntPairVal.second];
    llvm::Type *Ty = I.getType();
    switch (Ty->getTypeID()) {
    case llvm::Type::IntegerTyID:
        Dest.IntVal = Src.IntVal;
        break;
        IMPLEMENT_VAARG(Pointer);
        IMPLEMENT_VAARG(Float);
        IMPLEMENT_VAARG(Double);
    default:
        borealis::dbgs() << "Unhandled dest llvm::Type for vaarg instruction: " << *Ty << "\n";
        UNREACHABLE(nullptr);
    }

    // Set the Value of this Instruction.
    SetValue(&I, Dest, SF);

    // Move the pointer to the next vararg.
    ++VAList.UIntPairVal.second;
}

void InstructionExecutor::visitExtractElementInst(ExtractElementInst &I) {
    TRACE_FUNC;
    ExecutorContext& SF = ee->getCurrentContext();
    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Dest;

    llvm::Type *Ty = I.getType();
    const unsigned indx = unsigned(Src2.IntVal.getZExtValue());

    if(Src1.AggregateVal.size() > indx) {
        switch (Ty->getTypeID()) {
        default:
            borealis::dbgs() << "Unhandled destination llvm::Type for extractelement instruction: "
            << *Ty << "\n";
            UNREACHABLE(nullptr);
            break;
        case llvm::Type::IntegerTyID:
            Dest.IntVal = Src1.AggregateVal[indx].IntVal;
            break;
        case llvm::Type::FloatTyID:
            Dest.FloatVal = Src1.AggregateVal[indx].FloatVal;
            break;
        case llvm::Type::DoubleTyID:
            Dest.DoubleVal = Src1.AggregateVal[indx].DoubleVal;
            break;
        }
    } else {
        borealis::dbgs() << "Invalid index in extractelement instruction\n";
    }

    SetValue(&I, Dest, SF);
}

void InstructionExecutor::visitInsertElementInst(InsertElementInst &I) {
    TRACE_FUNC;
    ExecutorContext& SF = ee->getCurrentContext();
    llvm::Type *Ty = I.getType();

    if(!(Ty->isVectorTy()) )
        UNREACHABLE("Unhandled dest llvm::Type for insertelement instruction");

    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Src3 = ee->getOperandValue(I.getOperand(2), SF);
    GenericValue Dest;

    llvm::Type *TyContained = Ty->getContainedType(0);

    const unsigned indx = unsigned(Src3.IntVal.getZExtValue());
    Dest.AggregateVal = Src1.AggregateVal;

    if(Src1.AggregateVal.size() <= indx)
        UNREACHABLE("Invalid index in insertelement instruction");
    switch (TyContained->getTypeID()) {
    default:
        UNREACHABLE("Unhandled dest llvm::Type for insertelement instruction");
    case llvm::Type::IntegerTyID:
        Dest.AggregateVal[indx].IntVal = Src2.IntVal;
        break;
    case llvm::Type::FloatTyID:
        Dest.AggregateVal[indx].FloatVal = Src2.FloatVal;
        break;
    case llvm::Type::DoubleTyID:
        Dest.AggregateVal[indx].DoubleVal = Src2.DoubleVal;
        break;
    }
    SetValue(&I, Dest, SF);
}

void InstructionExecutor::visitShuffleVectorInst(ShuffleVectorInst &I){
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();

    llvm::Type *Ty = I.getType();
    if(!(Ty->isVectorTy()))
        UNREACHABLE("Unhandled dest llvm::Type for shufflevector instruction");

    GenericValue Src1 = ee->getOperandValue(I.getOperand(0), SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Src3 = ee->getOperandValue(I.getOperand(2), SF);
    GenericValue Dest;

    // There is no need to check Types of src1 and src2, because the compiled
    // bytecode can't contain different Types for src1 and src2 for a
    // shufflevector instruction.

    llvm::Type *TyContained = Ty->getContainedType(0);
    unsigned src1Size = (unsigned)Src1.AggregateVal.size();
    unsigned src2Size = (unsigned)Src2.AggregateVal.size();
    unsigned src3Size = (unsigned)Src3.AggregateVal.size();

    Dest.AggregateVal.resize(src3Size);

    switch (TyContained->getTypeID()) {
    default:
        UNREACHABLE("Unhandled dest llvm::Type for insertelement instruction");
        break;
    case llvm::Type::IntegerTyID:
        for( unsigned i=0; i<src3Size; i++) {
            unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
            if(j < src1Size)
                Dest.AggregateVal[i].IntVal = Src1.AggregateVal[j].IntVal;
            else if(j < src1Size + src2Size)
                Dest.AggregateVal[i].IntVal = Src2.AggregateVal[j-src1Size].IntVal;
            else
                // The selector may not be greater than sum of lengths of first and
                // second operands and llasm should not allow situation like
                // %tmp = shufflevector <2 x i32> <i32 3, i32 4>, <2 x i32> undef,
                //                      <2 x i32> < i32 0, i32 5 >,
                // where i32 5 is invalid, but let it be additional check here:
                UNREACHABLE("Invalid mask in shufflevector instruction");
        }
        break;
    case llvm::Type::FloatTyID:
        for( unsigned i=0; i<src3Size; i++) {
            unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
            if(j < src1Size)
                Dest.AggregateVal[i].FloatVal = Src1.AggregateVal[j].FloatVal;
            else if(j < src1Size + src2Size)
                Dest.AggregateVal[i].FloatVal = Src2.AggregateVal[j-src1Size].FloatVal;
            else
                UNREACHABLE("Invalid mask in shufflevector instruction");
        }
        break;
    case llvm::Type::DoubleTyID:
        for( unsigned i=0; i<src3Size; i++) {
            unsigned j = Src3.AggregateVal[i].IntVal.getZExtValue();
            if(j < src1Size)
                Dest.AggregateVal[i].DoubleVal = Src1.AggregateVal[j].DoubleVal;
            else if(j < src1Size + src2Size)
                Dest.AggregateVal[i].DoubleVal =
                    Src2.AggregateVal[j-src1Size].DoubleVal;
            else
                UNREACHABLE("Invalid mask in shufflevector instruction");
        }
        break;
    }
    SetValue(&I, Dest, SF);
}

void InstructionExecutor::visitExtractValueInst(ExtractValueInst &I) {
    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    Value *Agg = I.getAggregateOperand();
    GenericValue Dest;
    GenericValue Src = ee->getOperandValue(Agg, SF);

    ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
    unsigned Num = I.getNumIndices();
    GenericValue *pSrc = &Src;

    for (unsigned i = 0 ; i < Num; ++i) {
        pSrc = &pSrc->AggregateVal[*IdxBegin];
        ++IdxBegin;
    }

    llvm::Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());
    switch (IndexedType->getTypeID()) {
    default:
        UNREACHABLE("Unhandled dest llvm::Type for extractelement instruction");
        break;
    case llvm::Type::IntegerTyID:
        Dest.IntVal = pSrc->IntVal;
        break;
    case llvm::Type::FloatTyID:
        Dest.FloatVal = pSrc->FloatVal;
        break;
    case llvm::Type::DoubleTyID:
        Dest.DoubleVal = pSrc->DoubleVal;
        break;
    case llvm::Type::ArrayTyID:
    case llvm::Type::StructTyID:
    case llvm::Type::VectorTyID:
        Dest.AggregateVal = pSrc->AggregateVal;
        break;
    case llvm::Type::PointerTyID:
        Dest.PointerVal = pSrc->PointerVal;
        break;
    }

    SetValue(&I, Dest, SF);
}

void InstructionExecutor::visitInsertValueInst(InsertValueInst &I) {

    TRACE_FUNC;
    ExecutorContext &SF = ee->getCurrentContext();
    Value *Agg = I.getAggregateOperand();

    GenericValue Src1 = ee->getOperandValue(Agg, SF);
    GenericValue Src2 = ee->getOperandValue(I.getOperand(1), SF);
    GenericValue Dest = Src1; // Dest is a slightly changed Src1

    ExtractValueInst::idx_iterator IdxBegin = I.idx_begin();
    unsigned Num = I.getNumIndices();

    GenericValue *pDest = &Dest;
    for (unsigned i = 0 ; i < Num; ++i) {
        pDest = &pDest->AggregateVal[*IdxBegin];
        ++IdxBegin;
    }
    // pDest points to the target value in the Dest now

    llvm::Type *IndexedType = ExtractValueInst::getIndexedType(Agg->getType(), I.getIndices());

    switch (IndexedType->getTypeID()) {
    default:
        UNREACHABLE("Unhandled dest llvm::Type for insertelement instruction");
        break;
    case llvm::Type::IntegerTyID:
        pDest->IntVal = Src2.IntVal;
        break;
    case llvm::Type::FloatTyID:
        pDest->FloatVal = Src2.FloatVal;
        break;
    case llvm::Type::DoubleTyID:
        pDest->DoubleVal = Src2.DoubleVal;
        break;
    case llvm::Type::ArrayTyID:
    case llvm::Type::StructTyID:
    case llvm::Type::VectorTyID:
        pDest->AggregateVal = Src2.AggregateVal;
        break;
    case llvm::Type::PointerTyID:
        pDest->PointerVal = Src2.PointerVal;
        break;
    }

    SetValue(&I, Dest, SF);
}

GenericValue borealis::ExecutionEngine::getConstantExprValue (ConstantExpr *CE,
    ExecutorContext &SF) {
    switch (CE->getOpcode()) {
    case Instruction::Trunc:
        return executeTruncInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::ZExt:
        return executeZExtInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::SExt:
        return executeSExtInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::FPTrunc:
        return executeFPTruncInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::FPExt:
        return executeFPExtInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::UIToFP:
        return executeUIToFPInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::SIToFP:
        return executeSIToFPInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::FPToUI:
        return executeFPToUIInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::FPToSI:
        return executeFPToSIInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::PtrToInt:
        return executePtrToIntInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::IntToPtr:
        return executeIntToPtrInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::BitCast:
        return executeBitCastInst(CE->getOperand(0), CE->getType(), SF);
    case Instruction::GetElementPtr:
        return executeGEPOperation(CE->getOperand(0), gep_type_begin(CE),
            gep_type_end(CE), SF);
    case Instruction::FCmp:
    case Instruction::ICmp:
        return executeCmpInst(CE->getPredicate(),
            getOperandValue(CE->getOperand(0), SF),
            getOperandValue(CE->getOperand(1), SF),
            CE->getOperand(0)->getType());
    case Instruction::Select:
        return executeSelectInst(getOperandValue(CE->getOperand(0), SF),
            getOperandValue(CE->getOperand(1), SF),
            getOperandValue(CE->getOperand(2), SF),
            CE->getOperand(0)->getType());
    default :
        break;
    }

    // The cases below here require a GenericValue parameter for the result
    // so we initialize one, compute it and then return it.
    GenericValue Op0 = getOperandValue(CE->getOperand(0), SF);
    GenericValue Op1 = getOperandValue(CE->getOperand(1), SF);
    GenericValue Dest;
    llvm::Type * Ty = CE->getOperand(0)->getType();
    switch (CE->getOpcode()) {
    case Instruction::Add:  Dest.IntVal = Op0.IntVal + Op1.IntVal; break;
    case Instruction::Sub:  Dest.IntVal = Op0.IntVal - Op1.IntVal; break;
    case Instruction::Mul:  Dest.IntVal = Op0.IntVal * Op1.IntVal; break;
    case Instruction::FAdd: executeFAddInst(Dest, Op0, Op1, Ty); break;
    case Instruction::FSub: executeFSubInst(Dest, Op0, Op1, Ty); break;
    case Instruction::FMul: executeFMulInst(Dest, Op0, Op1, Ty); break;
    case Instruction::FDiv: executeFDivInst(Dest, Op0, Op1, Ty); break;
    case Instruction::FRem: executeFRemInst(Dest, Op0, Op1, Ty); break;
    case Instruction::SDiv: Dest.IntVal = Op0.IntVal.sdiv(Op1.IntVal); break;
    case Instruction::UDiv: Dest.IntVal = Op0.IntVal.udiv(Op1.IntVal); break;
    case Instruction::URem: Dest.IntVal = Op0.IntVal.urem(Op1.IntVal); break;
    case Instruction::SRem: Dest.IntVal = Op0.IntVal.srem(Op1.IntVal); break;
    case Instruction::And:  Dest.IntVal = Op0.IntVal & Op1.IntVal; break;
    case Instruction::Or:   Dest.IntVal = Op0.IntVal | Op1.IntVal; break;
    case Instruction::Xor:  Dest.IntVal = Op0.IntVal ^ Op1.IntVal; break;
    case Instruction::Shl:
        Dest.IntVal = Op0.IntVal.shl(Op1.IntVal.getZExtValue());
        break;
    case Instruction::LShr:
        Dest.IntVal = Op0.IntVal.lshr(Op1.IntVal.getZExtValue());
        break;
    case Instruction::AShr:
        Dest.IntVal = Op0.IntVal.ashr(Op1.IntVal.getZExtValue());
        break;
    default:
        borealis::dbgs() << "Unhandled ConstantExpr: " << *CE << "\n";
        UNREACHABLE("Unhandled ConstantExpr");
    }
    return Dest;
}

GenericValue borealis::ExecutionEngine::getOperandValue(Value *V, ExecutorContext &SF) {
    TRACE_FUNC;
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
        return getConstantExprValue(CE, SF);
    } else if (Constant *CPV = dyn_cast<Constant>(V)) {
        return Mem.getConstantValue(CPV);
    } else if (GlobalValue *GV = dyn_cast<GlobalValue>(V)) {
        return GenericValue{
            Mem.getPointerToGlobal(GV, TD->getTypeSizeInBits(GV->getType()->getPointerElementType()), 0)
        };
    } else if(auto gv = util::at(SF.Values, V)) {
        for(auto&& val : gv) return val;
    } else if(!V->getType()->isPointerTy()) {
        return Judicator->map(V);
    } else {
        return llvm::GenericValue{ Mem.getOpaquePtr() };
    }
    return {};
}

//===----------------------------------------------------------------------===//
//                        Dispatch and Execution Code
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// callFunction - Execute the specified function...
//
void borealis::ExecutionEngine::callFunction(Function *F,
    const std::vector<GenericValue> &ArgVals) {
    TRACE_FUNC;

    ASSERT((ECStack.empty() || !ECStack.back().Caller.getInstruction() ||
        ECStack.back().Caller.arg_size() == ArgVals.size()),
        "Incorrect number of arguments passed into function call!");
    // Make a new stack frame... and fill it in.
    ECStack.push_back(ExecutorContext());
    ExecutorContext &StackFrame = ECStack.back();
    StackFrame.CurFunction = F;

    // Special handling for external functions.
    if (F->isDeclaration()) {
        auto Result = callExternalFunction (F, ArgVals);
        // Simulate a 'ret' instruction of the appropriate llvm::Type.
        if(Result) popStackAndReturnValueToCaller (F->getReturnType (), Result.getUnsafe());
        else popStackAndReturnValueToCaller(nullptr, {});
        return;
    }

    // Get pointers to first LLVM BB & Instruction in function.
    StackFrame.CurBB     = F->begin();
    StackFrame.CurInst   = StackFrame.CurBB->begin();

    // Run through the function arguments and initialize their values...
    ASSERT((ArgVals.size() == F->arg_size() ||
        (ArgVals.size() > F->arg_size() && F->getFunctionType()->isVarArg())),
        "Invalid number of values passed to function invocation!");

    // Handle non-varargs arguments...
    unsigned i = 0;
    for (Function::arg_iterator AI = F->arg_begin(), E = F->arg_end();
        AI != E; ++AI, ++i)
        SetValue(AI, ArgVals[i], StackFrame);

    // Handle varargs arguments...
    StackFrame.VarArgs.assign(ArgVals.begin()+i, ArgVals.end());
}


void borealis::ExecutionEngine::run() {
    TRACE_FUNC;
    while (!ECStack.empty()) {
        // Interpret a single instruction & increment the "PC".
        ExecutorContext &SF = ECStack.back();  // Current stack frame
        Instruction &I = *SF.CurInst++;         // Increment before execute

        // Track the number of dynamic instructions executed.

        IE.visit(I);   // Dispatch to one of the visit* methods...
    }
}

#include "Util/unmacros.h"

#pragma GCC diagnostic pop
