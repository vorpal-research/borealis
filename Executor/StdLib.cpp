/*
 * StdLib.cpp
 *
 *  Created on: Feb 17, 2015
 *      Author: belyaev
 */
#include <Executor/ExecutionEngine.h>
#include "Logging/tracer.hpp"
#include "Codegen/intrinsics.h"
#include "Config/config.h"

#include <cmath>

#include <llvm/Target/TargetLibraryInfo.h>

using namespace borealis;
using namespace borealis::config;

namespace lfn = llvm::LibFunc;

#include "Util/macros.h"

inline llvm::GenericValue wrap(const llvm::APInt& intVal) {
    llvm::GenericValue RetVal;
    RetVal.IntVal = intVal;
    return RetVal;
}

inline llvm::GenericValue wrap(llvm::APInt&& intVal) {
    llvm::GenericValue RetVal;
    RetVal.IntVal = std::move(intVal);
    return RetVal;
}

inline llvm::GenericValue wrap(unsigned long long intVal, llvm::Type* type) {
    llvm::GenericValue RetVal;
    RetVal.IntVal = llvm::APInt{ type->getIntegerBitWidth(), intVal };
    return RetVal;
}

inline llvm::GenericValue wrap(bool bval) {
    return wrap(llvm::APInt{1, bval});
}

inline llvm::GenericValue& wrap(llvm::GenericValue& gv) { return gv; }
inline llvm::GenericValue&& wrap(llvm::GenericValue&& gv) { return std::move(gv); }

template<class F>
inline llvm::GenericValue callFloat(
    F code,
    llvm::Type* type,
    const std::vector<llvm::GenericValue>& ArgVals) {
    llvm::GenericValue RetVal;
    if(type->isFloatTy()) {
        RetVal.FloatVal = code(ArgVals.back().FloatVal);
    } else if(type -> isDoubleTy()) {
        RetVal.DoubleVal = code(ArgVals.back().DoubleVal);
    } else if(type->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        UNREACHABLE("Unsupported type");
    }
    return RetVal;
}

template<class F>
inline llvm::GenericValue callFloat2(
    F code,
    llvm::Type* type,
    const std::vector<llvm::GenericValue>& ArgVals) {
    llvm::GenericValue RetVal;
    if(type->isFloatTy()) {
        RetVal.FloatVal = code(ArgVals[0].FloatVal, ArgVals[1].FloatVal);
    } else if(type -> isDoubleTy()) {
        RetVal.DoubleVal = code(ArgVals[0].DoubleVal, ArgVals[1].DoubleVal);
    } else if(type->isFloatingPointTy()){
        UNREACHABLE("Unsupported floating point type");
    } else {
        UNREACHABLE("Unsupported type");
    }
    return RetVal;
}

using GenericFunc = llvm::GenericValue(const std::vector<llvm::GenericValue>&, const llvm::Function*);
using PGenericFunc = GenericFunc*;

#define STDLIB(...) [](const std::vector<llvm::GenericValue>& ArgVals, const llvm::Function* F){ return wrap(__VA_ARGS__); }

static const std::unordered_map<int, PGenericFunc> stdLibArith() {
    using util::just;
    using util::nothing;

    static const std::unordered_map<int, PGenericFunc> allStdLibArithFuncs = [](){
        std::unordered_map<int, PGenericFunc> retVal;
        retVal[lfn::abs] =
        retVal[lfn::llabs] =
        retVal[lfn::labs] = STDLIB(ArgVals.back().IntVal.abs());

        retVal[lfn::ffs] =
        retVal[lfn::ffsl] =
        retVal[lfn::ffsll] = STDLIB(ArgVals.back().IntVal.countLeadingZeros(), F->getReturnType());

        retVal[lfn::htonl] =
        retVal[lfn::ntohl] =
        retVal[lfn::htons] =
        retVal[lfn::ntohs] = STDLIB(ArgVals.back().IntVal.byteSwap());

        retVal[lfn::isascii] = STDLIB(
            !!std::isalpha(static_cast<int>(ArgVals.back().IntVal.getLimitedValue(256)))
        );
        retVal[lfn::isdigit] = STDLIB(
            !!std::isdigit(static_cast<int>(ArgVals.back().IntVal.getLimitedValue(256)))
        );

#define FLOAT_FWD_TO_HOST(FNAME) \
        retVal[lfn::FNAME] = \
        retVal[lfn::FNAME##f] = \
        retVal[lfn::FNAME##l] = STDLIB(callFloat(LAM(x, std::FNAME(x)), F->getReturnType(), ArgVals));

        FLOAT_FWD_TO_HOST(acos);
        FLOAT_FWD_TO_HOST(acosh);
        FLOAT_FWD_TO_HOST(asin);
        FLOAT_FWD_TO_HOST(asinh);
        FLOAT_FWD_TO_HOST(atan);
        FLOAT_FWD_TO_HOST(atanh);
        FLOAT_FWD_TO_HOST(cbrt);
        FLOAT_FWD_TO_HOST(ceil);
        FLOAT_FWD_TO_HOST(cos);
        FLOAT_FWD_TO_HOST(cosh);
        FLOAT_FWD_TO_HOST(exp);
        FLOAT_FWD_TO_HOST(exp2);
        FLOAT_FWD_TO_HOST(expm1);
        FLOAT_FWD_TO_HOST(fabs);
        FLOAT_FWD_TO_HOST(floor);
        FLOAT_FWD_TO_HOST(round);
        FLOAT_FWD_TO_HOST(rint);
        FLOAT_FWD_TO_HOST(nearbyint);
        FLOAT_FWD_TO_HOST(log);
        FLOAT_FWD_TO_HOST(log2);
        FLOAT_FWD_TO_HOST(log10);
        FLOAT_FWD_TO_HOST(logb);
        FLOAT_FWD_TO_HOST(log1p);
        FLOAT_FWD_TO_HOST(sin);
        FLOAT_FWD_TO_HOST(sinh);
        FLOAT_FWD_TO_HOST(sqrt);
        FLOAT_FWD_TO_HOST(tan);
        FLOAT_FWD_TO_HOST(tanh);
        FLOAT_FWD_TO_HOST(trunc);

#undef FLOAT_FWD_TO_HOST
#define FLOAT2_FWD_TO_HOST(FNAME) \
        retVal[lfn::FNAME] = \
        retVal[lfn::FNAME##f] = \
        retVal[lfn::FNAME##l] = STDLIB(callFloat2(LAM2(x, y, std::FNAME(x, y)), F->getReturnType(), ArgVals));

        FLOAT2_FWD_TO_HOST(fmax);
        FLOAT2_FWD_TO_HOST(fmin);
        FLOAT2_FWD_TO_HOST(fmod);
        FLOAT2_FWD_TO_HOST(atan2);
        FLOAT2_FWD_TO_HOST(copysign);
        FLOAT2_FWD_TO_HOST(pow);

#undef FLOAT2_FWD_TO_HOST

        return std::move(retVal);
    }();
    return allStdLibArithFuncs;
}

static llvm::GenericValue executeStrLen(
        const llvm::Function* F,
        const std::vector<llvm::GenericValue>& ArgVals,
        MemorySimulator& Mem) {
    TRACE_FUNC;

    using util::just;
    using util::nothing;

    auto ptr = ArgVals[0].PointerVal;
    llvm::GenericValue RetVal;
    auto fnd = (Mem.MemChr(ptr, 0, ~size_t(0)));
    RetVal.IntVal = llvm::APInt(F->getReturnType()->getIntegerBitWidth(), static_cast<uint8_t*>(fnd) - static_cast<uint8_t*>(ptr));
    TRACE_PARAM(RetVal.IntVal);
    return RetVal;
}

util::option<llvm::GenericValue> ExecutionEngine::callStdLibFunction(
    const llvm::Function* F,
    const std::vector<llvm::GenericValue>& ArgVals) {
    using util::just;
    using util::nothing;

    TRACE_FUNC;

    lfn::Func fcode;
    TLI->getLibFunc(F->getName(), fcode);

    switch(fcode) {
    /// double frexp(double num, int *exp);
    case lfn::frexp:
    /// float frexpf(float num, int *exp);
    case lfn::frexpf:
    /// long double frexpl(long double num, int *exp);
    case lfn::frexpl: {
        int exp;
        auto res = callFloat(LAM(x, std::frexp(x, &exp)), F->getReturnType(), ArgVals);
        llvm::APInt dynExp{ F->getFunctionType()->getFunctionParamType(1)->getIntegerBitWidth(), (uint64_t)exp };
        llvm::MutableArrayRef<uint8_t> where {
            static_cast<uint8_t*>(ArgVals[1].PointerVal),
            (dynExp.getBitWidth() + 7)/8
        };
        Mem.StoreIntToMemory(dynExp, where);
        return just(res);
    }
    /// double ldexp(double x, int n);
    case lfn::ldexp:
    /// float ldexpf(float x, int n);
    case lfn::ldexpf:
    /// long double ldexpl(long double x, int n);
    case lfn::ldexpl: {
        auto exp = ArgVals[1].IntVal.getSExtValue();
        return just(callFloat(LAM(x, std::ldexp(x, static_cast<int>(exp))), F->getReturnType(), ArgVals));
    }
    /// double modf(double x, double *iptr);
    case lfn::modf:
    /// float modff(float, float *iptr);
    case lfn::modff:
    /// long double modfl(long double value, long double *iptr);
    case lfn::modfl:{
        auto tp = F->getReturnType();
        if(tp -> isFloatTy()) {
            llvm::GenericValue ip, RetVal;
            auto res = std::modf(ArgVals[0].FloatVal, &ip.FloatVal);
            StoreValueToMemory(ip, (uint8_t*)ArgVals[1].PointerVal, F->getFunctionType()->getFunctionParamType(1));
            RetVal.FloatVal = res;
            return just(RetVal);
        } else if(tp -> isDoubleTy()) {
            llvm::GenericValue ip, RetVal;
            auto res = std::modf(ArgVals[0].DoubleVal, &ip.DoubleVal);
            StoreValueToMemory(ip, (uint8_t*)ArgVals[1].PointerVal, F->getFunctionType()->getFunctionParamType(1));
            RetVal.DoubleVal = res;
            return just(RetVal);
        } else if(tp -> isFloatingPointTy()){
            UNREACHABLE("Unsupported floating point type");
        } else {
            UNREACHABLE("Unsupported type");
        }
        break;
    }

    case lfn::memchr: {
        TRACE_BLOCK("Running memchr");
        auto ptr = ArgVals[0].PointerVal;
        auto ch = static_cast<uint8_t>(ArgVals[1].IntVal.getLimitedValue(255));
        auto limit = ArgVals[2].IntVal.getLimitedValue();
        llvm::GenericValue RetVal;
        RetVal.PointerVal = Mem.MemChr(ptr, ch, limit);
        return just(RetVal);
    }

    case lfn::strlen: {
        return just(executeStrLen(F, ArgVals, Mem));
    }

    case lfn::strchr: {
        auto strlen = executeStrLen(F, ArgVals, Mem);
        auto ptr = ArgVals[0].PointerVal;
        auto ch = static_cast<uint8_t>(ArgVals[1].IntVal.getLimitedValue(255));
        auto limit = static_cast<size_t>(strlen.IntVal.getLimitedValue());

        llvm::GenericValue RetVal;
        RetVal.PointerVal = Mem.MemChr(ptr, ch, limit);
        return just(RetVal);
    }

    case lfn::strcpy: {
        auto argValsCopy = ArgVals;
        auto strlen = executeStrLen(F, ArgVals, Mem);
        argValsCopy.push_back(strlen);
        executeMemcpy(F, argValsCopy);
    };

    default:
        for(auto&& realFunction : util::at(stdLibArith(), fcode)) return just(realFunction(ArgVals, F));
        return nothing();
    }

    return nothing();
}

#include "Util/unmacros.h"

