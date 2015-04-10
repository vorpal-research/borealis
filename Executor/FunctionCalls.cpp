/*
 * FunctionCalls.cpp
 *
 *  Created on: Feb 5, 2015
 *      Author: belyaev
 */
#include <Executor/ExecutionEngine.h>
#include "Executor/Exceptions.h"
#include "Logging/tracer.hpp"
#include "Codegen/intrinsics.h"
#include "Config/config.h"

#include <cmath>

#include <llvm/Target/TargetLibraryInfo.h>

using namespace borealis;
using namespace borealis::config;

static ConfigEntry<int> MemcpyThreshold{"executor", "memcpy-exec-factor"};

namespace lfn = llvm::LibFunc;

#include "Util/macros.h"

static bool isStdLib(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC

    lfn::Func f;
    return TLI->getLibFunc(F->getName(), f);
}

static bool isMalloc(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC

    lfn::Func f;
    return TLI->getLibFunc(F->getName(), f) &&
         (f ==  lfn::malloc             ||
          f ==  lfn::valloc             ||
          f ==  lfn::Znwj               ||
          f ==  lfn::ZnwjRKSt9nothrow_t ||
          f ==  lfn::Znwm               ||
          f ==  lfn::ZnwmRKSt9nothrow_t ||
          f ==  lfn::Znaj               ||
          f ==  lfn::ZnajRKSt9nothrow_t ||
          f ==  lfn::Znam               ||
          f ==  lfn::ZnamRKSt9nothrow_t );
}

static bool isCalloc(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC
    lfn::Func f;
    return TLI->getLibFunc(F->getName(), f) &&
         (f ==  lfn::calloc);
}

static bool isRealloc(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC
    lfn::Func f;
    return TLI->getLibFunc(F->getName(), f) &&
         (f ==  lfn::realloc || f == lfn::reallocf);
}

static bool isFree(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC
    lfn::Func f;
    return TLI->getLibFunc(F->getName(), f) &&
         (f ==  lfn::free);
}

static bool isMemcpy(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC
    lfn::Func f;
    return F->getIntrinsicID() == llvm::Intrinsic::memcpy ||
        (TLI->getLibFunc(F->getName(), f) && (f ==  lfn::memcpy));
}

static bool isMemset(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC
    lfn::Func f;
    return F->getIntrinsicID() == llvm::Intrinsic::memset ||
        (TLI->getLibFunc(F->getName(), f) && (f ==  lfn::memset));
}

static bool isMemmove(const llvm::Function* F, const llvm::TargetLibraryInfo* TLI) {
    TRACE_FUNC
    lfn::Func f;
    return F->getIntrinsicID() == llvm::Intrinsic::memmove ||
        (TLI->getLibFunc(F->getName(), f) && (f ==  lfn::memmove));
}

static BoolConfigEntry NullableMallocs("analysis", "nullable-mallocs");

llvm::GenericValue ExecutionEngine::executeMalloc(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC

    // FIXME: different versions of malloc have different parameters, check
    if(NullableMallocs.get(true)) {
        auto predictedResult = Judicator->map(getCallerContext().Caller.getInstruction());
        if (predictedResult.PointerVal == nullptr) {
            return predictedResult;
        }
    }

    llvm::GenericValue RetVal{};
    auto sz = TD->getTypeStoreSize(f->getReturnType()->getPointerElementType());
    RetVal.PointerVal = Mem.MallocMemory(sz * ArgVals[0].IntVal.getLimitedValue(), MemorySimulator::MallocFill::UNINIT);
    return RetVal;
}
llvm::GenericValue ExecutionEngine::executeCalloc(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC

    llvm::GenericValue RetVal{};
    auto sz = TD->getTypeStoreSize(f->getReturnType()->getPointerElementType());
    RetVal.PointerVal = Mem.MallocMemory(sz * ArgVals[0].IntVal.getLimitedValue(), MemorySimulator::MallocFill::ZERO);
    return RetVal;
}
llvm::GenericValue ExecutionEngine::executeRealloc(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    return executeMalloc(f, ArgVals);
}
llvm::GenericValue ExecutionEngine::executeFree(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    // TODO

    Mem.FreeMemory(ArgVals[0].PointerVal);

    return {};
}
llvm::GenericValue ExecutionEngine::executeMemcpy(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;

    auto size = ArgVals.at(2).IntVal.getLimitedValue();
    if(size <= MemcpyThreshold.get(1024U) * Mem.getQuant()) {
        auto Dst = static_cast<uint8_t*>(ArgVals[0].PointerVal);
        auto Src = static_cast<uint8_t*>(ArgVals[1].PointerVal);

        std::vector<uint8_t> buf(size);
        llvm::MutableArrayRef<uint8_t> to{ Dst, size };
        llvm::ArrayRef<uint8_t> from{ Src, size };

        Mem.LoadBytesFromMemory(buf, from);
        Mem.StoreBytesToMemory(buf, to);
        return ArgVals.at(0);
    }
    throw std::logic_error("full memcpy not implemented yet");
    return {};
}
llvm::GenericValue ExecutionEngine::executeMemset(const llvm::Function*, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;

    Mem.Memset(
        ArgVals.at(0).PointerVal,
        static_cast<uint8_t>(ArgVals.at(1).IntVal.getLimitedValue()),
        static_cast<size_t>(ArgVals.at(2).IntVal.getLimitedValue())
    );

    return ArgVals.at(0);
}
llvm::GenericValue ExecutionEngine::executeMemmove(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC
    return executeMemcpy(f, ArgVals);
}

static llvm::GenericValue runSqrt(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    llvm::GenericValue RetVal;
    if(f->getReturnType()->isFloatTy()) {
        RetVal.FloatVal = std::sqrt(ArgVals.back().FloatVal);
    } else if(f->getReturnType()->isDoubleTy()) {
        RetVal.DoubleVal = std::sqrt(ArgVals.back().DoubleVal);
    } else if(f->getReturnType()->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        ASSERTC(f->getReturnType()->isIntegerTy())
        RetVal.IntVal = ArgVals.back().IntVal.sqrt();
    }
    return RetVal;
}

template<class F>
static llvm::GenericValue runFloatMath(F code, const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    llvm::GenericValue RetVal;
    if(f->getReturnType()->isFloatTy()) {
        RetVal.FloatVal = code(ArgVals.back().FloatVal);
    } else if(f->getReturnType()->isDoubleTy()) {
        RetVal.DoubleVal = code(ArgVals.back().DoubleVal);
    } else if(f->getReturnType()->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        UNREACHABLE("Unsupported type");
    }
    return RetVal;
}

static llvm::GenericValue runFMA(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    auto fma = [](auto&& a, auto&& b, auto&& c) { return a * b + c; };
    llvm::GenericValue RetVal;
    if(f->getReturnType()->isFloatTy()) {
        RetVal.FloatVal = fma(ArgVals[0].FloatVal, ArgVals[1].FloatVal, ArgVals[2].FloatVal);
    } else if(f->getReturnType()->isDoubleTy()) {
        RetVal.DoubleVal = fma(ArgVals[0].DoubleVal, ArgVals[1].DoubleVal, ArgVals[2].DoubleVal);
    } else if(f->getReturnType()->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        UNREACHABLE("Unsupported type");
    }
    return RetVal;
}

static llvm::GenericValue runpow(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    auto fma = [](auto&& a, auto&& b) { return std::pow(a, b); };
    llvm::GenericValue RetVal;
    if(f->getReturnType()->isFloatTy()) {
        RetVal.FloatVal = fma(ArgVals[0].FloatVal, ArgVals[1].FloatVal);
    } else if(f->getReturnType()->isDoubleTy()) {
        RetVal.DoubleVal = fma(ArgVals[0].DoubleVal, ArgVals[1].DoubleVal);
    } else if(f->getReturnType()->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        UNREACHABLE("Unsupported type");
    }
    return RetVal;
}

static llvm::GenericValue runpowi(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    auto fma = [](auto&& a, auto&& b) { return std::pow(a, b); };
    llvm::GenericValue RetVal;
    if(f->getReturnType()->isFloatTy()) {
        RetVal.FloatVal = fma(ArgVals[0].FloatVal, ArgVals[1].IntVal.getLimitedValue());
    } else if(f->getReturnType()->isDoubleTy()) {
        RetVal.DoubleVal = fma(ArgVals[0].DoubleVal, ArgVals[1].IntVal.getLimitedValue());
    } else if(f->getReturnType()->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        UNREACHABLE("Unsupported type");
    }
    return RetVal;
}

template<class F>
static llvm::GenericValue runIntegralWithOverflow(
        F code,
        const llvm::Function*,
        const std::vector<llvm::GenericValue> &ArgVals) {
    llvm::GenericValue RetVal;
    bool overflow = false;
    auto res = code(ArgVals[0].IntVal, ArgVals[1].IntVal, overflow);
    RetVal.AggregateVal.emplace_back();
    RetVal.AggregateVal.emplace_back();
    RetVal.AggregateVal[0].IntVal = res;
    RetVal.AggregateVal[1].IntVal = llvm::APInt{ 1, overflow };
    return RetVal;
}


util::option<llvm::GenericValue> ExecutionEngine::callExternalFunction(
    llvm::Function *F,
    const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    TRACE_PARAM(F->getName());
    using util::just;
    using util::nothing;

    // TODO
#define CHECK_AND_EXEC(NAME) if(is ## NAME(F, TLI)) return just(execute ## NAME (F, ArgVals));
    CHECK_AND_EXEC(Malloc)
    CHECK_AND_EXEC(Calloc)
    CHECK_AND_EXEC(Realloc)
    CHECK_AND_EXEC(Free)
    CHECK_AND_EXEC(Memcpy)
    CHECK_AND_EXEC(Memset)
    CHECK_AND_EXEC(Memmove)
#undef CHECK_AND_EXEC

    if(IM->getIntrinsicType(F) == function_type::BUILTIN_BOR_ASSERT) {
        TRACES() << "Assertion discovered, checking" << endl;
        if(!ArgVals[0].IntVal.getLimitedValue()) {
            throw assertion_failed{ llvm::valueSummary(getCallerContext().Caller.getInstruction()) };
        }
        return util::nothing();
    }

    if(IM->getIntrinsicType(F) == function_type::INTRINSIC_MALLOC) {
        std::vector<llvm::GenericValue> adjustedArgs;
        adjustedArgs.push_back(ArgVals[2]);
        return just(executeMalloc(F, adjustedArgs));
    }

    llvm::GenericValue RetVal;

    if(IM->getIntrinsicType(F) == function_type::INTRINSIC_ALLOC) {
        TRACE_BLOCK("function_type::INTRINSIC_ALLOC")
        RetVal.PointerVal = Mem.AllocateMemory(
            ArgVals[2].IntVal.getLimitedValue() * TD->getTypeStoreSize(F->getReturnType()->getPointerElementType())
        );
        return just(RetVal);
    }
    // LLVM intrinsics

#define GENERATE_STD(FNAME) \
    case llvm::Intrinsic:: FNAME : \
        return just(runFloatMath(LAM(v, std :: FNAME (v)), F, ArgVals));

#define GENERATE_INTEGRAL_WITH_OV(INTR) \
    case llvm::Intrinsic:: INTR ## _with_overflow : \
        return just(runIntegralWithOverflow([](auto&& lhv, auto&& rhv, bool& ov){ \
            return lhv . INTR ## _ov (rhv, ov); \
        }, F, ArgVals));

    switch(F->getIntrinsicID()) {
    default:
        UNREACHABLE("Tassadar: unsupported llvm intrinsic");
        break;
    case llvm::Intrinsic::not_intrinsic:
        break;

    case llvm::Intrinsic::bswap:
        RetVal.IntVal = ArgVals.back().IntVal.byteSwap();
        return just(RetVal);
    case llvm::Intrinsic::fma:
    case llvm::Intrinsic::fmuladd:
        return just(runFMA(F, ArgVals));
    case llvm::Intrinsic::pow:
        return just(runpow(F, ArgVals));
    case llvm::Intrinsic::powi:
        return just(runpowi(F, ArgVals));

    GENERATE_STD(ceil)
    GENERATE_STD(cos)
    GENERATE_STD(exp)
    GENERATE_STD(exp2)
    GENERATE_STD(fabs)
    GENERATE_STD(floor)
    GENERATE_STD(log)
    GENERATE_STD(log10)
    GENERATE_STD(log2)
    GENERATE_STD(sin)
    GENERATE_STD(sqrt)
    GENERATE_STD(trunc)
    GENERATE_STD(nearbyint)

    GENERATE_INTEGRAL_WITH_OV(sadd)
    GENERATE_INTEGRAL_WITH_OV(smul)
    GENERATE_INTEGRAL_WITH_OV(ssub)
    GENERATE_INTEGRAL_WITH_OV(uadd)
    GENERATE_INTEGRAL_WITH_OV(umul)
    GENERATE_INTEGRAL_WITH_OV(usub)
    }

#undef GENERATE_STD
#undef GENERATE_INTEGRAL_WITH_OV

    {
        if(isStdLib(F, TLI)) return callStdLibFunction(F , ArgVals);
    }
    return nothing();
}

#include "Util/unmacros.h"
