/*
 * StdLib.cpp
 *
 *  Created on: Feb 17, 2015
 *      Author: belyaev
 */
#include "Executor/Executor.h"
#include "Logging/tracer.hpp"
#include "Codegen/intrinsics.h"
#include "Config/config.h"

#include <cmath>

#include <llvm/Target/TargetLibraryInfo.h>

using namespace borealis;
using namespace borealis::config;

namespace lfn = llvm::LibFunc;

#include "Util/macros.h"

static llvm::GenericValue wrap(const llvm::APInt& intVal) {
    llvm::GenericValue RetVal;
    RetVal.IntVal = intVal;
    return RetVal;
}

static llvm::GenericValue wrap(llvm::APInt&& intVal) {
    llvm::GenericValue RetVal;
    RetVal.IntVal = std::move(intVal);
    return RetVal;
}

static llvm::GenericValue wrap(unsigned long long intVal, llvm::Type* type) {
    llvm::GenericValue RetVal;
    RetVal.IntVal = llvm::APInt{ type->getIntegerBitWidth(), intVal };
    return RetVal;
}

static llvm::GenericValue wrap(bool bval) {
    return wrap(llvm::APInt{1, bval});
}

template<class F>
static llvm::GenericValue callFloat(
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
static llvm::GenericValue callFloat2(
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

util::option<llvm::GenericValue> Executor::callStdLibFunction(
    const llvm::Function* F,
    const std::vector<llvm::GenericValue>& ArgVals) {
    using util::just;
    using util::nothing;

    lfn::Func fcode;
    TLI->getLibFunc(F->getName(), fcode);

    switch(fcode) {
    /// int functions
    /// int abs(int j);
    case lfn::abs:
    /// long long int llabs(long long int j);
    case lfn::llabs:
    /// long int labs(long int j);
    case lfn::labs:
        return just(wrap(ArgVals.back().IntVal.abs()));

    /// int ffs(int i);
    case lfn::ffs:
    /// int ffsl(long int i);
    case lfn::ffsl:
    /// int ffsll(long long int i);
    case lfn::ffsll:
        return just(wrap(ArgVals.back().IntVal.countLeadingZeros(), F->getReturnType()));

    /// uint32_t htonl(uint32_t hostlong);
    case lfn::htonl:
    /// uint16_t htons(uint16_t hostshort);
    case lfn::htons:
    /// uint32_t ntohl(uint32_t netlong);
    case lfn::ntohl:
    /// uint16_t ntohs(uint16_t netshort);
    case lfn::ntohs:
        return just(wrap(ArgVals.back().IntVal.byteSwap()));

    /// int isascii(int c);
    case lfn::isascii: {
        auto v = ArgVals.back().IntVal.getLimitedValue(256);
        return just(wrap(!!std::isalpha(static_cast<int>(v))));
    }
    /// int isdigit(int c);
    case lfn::isdigit:{
        auto v = ArgVals.back().IntVal.getLimitedValue(256);
        return just(wrap(!!std::isdigit(static_cast<int>(v))));
    }

    /// double acos(double x);
    case lfn::acos:
    /// float acosf(float x);
    case lfn::acosf:
    /// long double acosl(long double x);
    case lfn::acosl:
        return just(callFloat(LAM(x, std::acos(x)), F->getReturnType(), ArgVals));

    /// double acosh(double x);
    case lfn::acosh:
    /// float acoshf(float x);
    case lfn::acoshf:
    /// long double acoshl(long double x);
    case lfn::acoshl:
        return just(callFloat(LAM(x, std::acosh(x)), F->getReturnType(), ArgVals));

    /// double asin(double x);
    case lfn::asin:
    /// float asinf(float x);
    case lfn::asinf:
    /// long double asinl(long double x);
    case lfn::asinl:
        return just(callFloat(LAM(x, std::asin(x)), F->getReturnType(), ArgVals));

    /// double asinh(double x);
    case lfn::asinh:
    /// float asinhf(float x);
    case lfn::asinhf:
    /// long double asinhl(long double x);
    case lfn::asinhl:
        return just(callFloat(LAM(x, std::asinh(x)), F->getReturnType(), ArgVals));

    /// double atan(double x);
    case lfn::atan:
    /// float atanf(float x);
    case lfn::atanf:
    /// long double atanl(long double x);
    case lfn::atanl:
        return just(callFloat(LAM(x, std::atan(x)), F->getReturnType(), ArgVals));

    /// double atanh(double x);
    case lfn::atanh:
    /// float atanhf(float x);
    case lfn::atanhf:
    /// long double atanhl(long double x);
    case lfn::atanhl:
        return just(callFloat(LAM(x, std::atanh(x)), F->getReturnType(), ArgVals));

    /// double cbrt(double x);
    case lfn::cbrt:
    /// float cbrtf(float x);
    case lfn::cbrtf:
    /// long double cbrtl(long double x);
    case lfn::cbrtl:
        return just(callFloat(LAM(x, std::cbrt(x)), F->getReturnType(), ArgVals));

    /// double ceil(double x);
    case lfn::ceil:
    /// float ceilf(float x);
    case lfn::ceilf:
    /// long double ceill(long double x);
    case lfn::ceill:
        return just(callFloat(LAM(x, std::ceil(x)), F->getReturnType(), ArgVals));

    /// double cos(double x);
    case lfn::cos:
    /// float cosf(float x);
    case lfn::cosf:
    /// long double cosl(long double x);
    case lfn::cosl:
        return just(callFloat(LAM(x, std::cos(x)), F->getReturnType(), ArgVals));

    /// double cosh(double x);
    case lfn::cosh:
    /// float coshf(float x);
    case lfn::coshf:
    /// long double coshl(long double x);
    case lfn::coshl:
        return just(callFloat(LAM(x, std::cosh(x)), F->getReturnType(), ArgVals));

    /// double exp(double x);
    case lfn::exp:
    /// float expf(float x);
    case lfn::expf:
    /// long double expl(long double x);
    case lfn::expl:
        return just(callFloat(LAM(x, std::exp(x)), F->getReturnType(), ArgVals));

    /// double exp10(double x);
    case lfn::exp10:
    /// float exp10f(float x);
    case lfn::exp10f:
    /// long double exp10l(long double x);
    case lfn::exp10l:
        return just(callFloat(LAM(x, std::pow(10, x)), F->getReturnType(), ArgVals));

    /// double exp2(double x);
    case lfn::exp2:
    /// float exp2f(float x);
    case lfn::exp2f:
    /// long double exp2l(long double x);
    case lfn::exp2l:
        return just(callFloat(LAM(x, std::exp2(x)), F->getReturnType(), ArgVals));

    /// double expm1(double x);
    case lfn::expm1:
    /// float expm1f(float x);
    case lfn::expm1f:
    /// long double expm1l(long double x);
    case lfn::expm1l:
        return just(callFloat(LAM(x, std::expm1(x)), F->getReturnType(), ArgVals));

    /// double fabs(double x);
    case lfn::fabs:
    /// float fabsf(float x);
    case lfn::fabsf:
    /// long double fabsl(long double x);
    case lfn::fabsl:
        return just(callFloat(LAM(x, std::fabs(x)), F->getReturnType(), ArgVals));

    /// double floor(double x);
    case lfn::floor:
    /// float floorf(float x);
    case lfn::floorf:
    /// long double floorl(long double x);
    case lfn::floorl:
        return just(callFloat(LAM(x, std::floor(x)), F->getReturnType(), ArgVals));

    /// double fmax(double x, double y);
    case lfn::fmax:
    /// float fmaxf(float x, float y);
    case lfn::fmaxf:
    /// long double fmaxl(long double x, long double y);
    case lfn::fmaxl:
        return just(callFloat2(LAM2(x, y, std::fmax(x, y)), F->getReturnType(), ArgVals));

    /// double fmin(double x, double y);
    case lfn::fmin:
    /// float fminf(float x, float y);
    case lfn::fminf:
    /// long double fminl(long double x, long double y);
    case lfn::fminl:
        return just(callFloat2(LAM2(x, y, std::fmin(x, y)), F->getReturnType(), ArgVals));

    /// double fmod(double x, double y);
    case lfn::fmod:
    /// float fmodf(float x, float y);
    case lfn::fmodf:
    /// long double fmodl(long double x, long double y);
    case lfn::fmodl:
        return just(callFloat2(LAM2(x, y, std::fmod(x, y)), F->getReturnType(), ArgVals));

    /// double log(double x);
    case lfn::log:
    /// float logf(float x);
    case lfn::logf:
    /// long double logl(long double x);
    case lfn::logl:
        return just(callFloat(LAM(x, std::log(x)), F->getReturnType(), ArgVals));

    /// double log10(double x);
    case lfn::log10:
    /// float log10f(float x);
    case lfn::log10f:
    /// long double log10l(long double x);
    case lfn::log10l:
        return just(callFloat(LAM(x, std::log10(x)), F->getReturnType(), ArgVals));

    /// double log1p(double x);
    case lfn::log1p:
    /// float log1pf(float x);
    case lfn::log1pf:
    /// long double log1pl(long double x);
    case lfn::log1pl:
        return just(callFloat(LAM(x, std::log1p(x)), F->getReturnType(), ArgVals));

    /// double log2(double x);
    case lfn::log2:
    /// float log2f(float x);
    case lfn::log2f:
    /// double long double log2l(long double x);
    case lfn::log2l:
        return just(callFloat(LAM(x, std::log2(x)), F->getReturnType(), ArgVals));

    /// double logb(double x);
    case lfn::logb:
    /// float logbf(float x);
    case lfn::logbf:
    /// long double logbl(long double x);
    case lfn::logbl:
        return just(callFloat(LAM(x, std::logb(x)), F->getReturnType(), ArgVals));

    /// double nearbyint(double x);
    case lfn::nearbyint:
    /// float nearbyintf(float x);
    case lfn::nearbyintf:
    /// long double nearbyintl(long double x);
    case lfn::nearbyintl:
        return just(callFloat(LAM(x, std::nearbyint(x)), F->getReturnType(), ArgVals));

    /// double rint(double x);
    case lfn::rint:
    /// float rintf(float x);
    case lfn::rintf:
    /// long double rintl(long double x);
    case lfn::rintl:
        return just(callFloat(LAM(x, std::rint(x)), F->getReturnType(), ArgVals));

    /// double round(double x);
    case lfn::round:
    /// float roundf(float x);
    case lfn::roundf:
    /// long double roundl(long double x);
    case lfn::roundl:
        return just(callFloat(LAM(x, std::round(x)), F->getReturnType(), ArgVals));

    /// double sin(double x);
    case lfn::sin:
    /// float sinf(float x);
    case lfn::sinf:
    /// long double sinl(long double x);
    case lfn::sinl:
        return just(callFloat(LAM(x, std::sin(x)), F->getReturnType(), ArgVals));

        /// double sinh(double x);
    case lfn::sinh:
    /// float sinhf(float x);
    case lfn::sinhf:
    /// long double sinhl(long double x);
    case lfn::sinhl:
        return just(callFloat(LAM(x, std::sinh(x)), F->getReturnType(), ArgVals));

    /// double sqrt(double x);
    case lfn::sqrt:
    /// float sqrtf(float x);
    case lfn::sqrtf:
    /// long double sqrtl(long double x);
    case lfn::sqrtl:
        return just(callFloat(LAM(x, std::sqrt(x)), F->getReturnType(), ArgVals));

    /// double tan(double x);
    case lfn::tan:
    /// float tanf(float x);
    case lfn::tanf:
    /// long double tanl(long double x);
    case lfn::tanl:
        return just(callFloat(LAM(x, std::tan(x)), F->getReturnType(), ArgVals));

    /// double tanh(double x);
    case lfn::tanh:
    /// float tanhf(float x);
    case lfn::tanhf:
    /// long double tanhl(long double x);
    case lfn::tanhl:
        return just(callFloat(LAM(x, std::tanh(x)), F->getReturnType(), ArgVals));

    /// double trunc(double x);
    case lfn::trunc:
    /// float truncf(float x);
    case lfn::truncf:
    /// long double truncl(long double x);
    case lfn::truncl:
        return just(callFloat(LAM(x, std::trunc(x)), F->getReturnType(), ArgVals));

    /// double atan2(double y, double x);
    case lfn::atan2:
    /// float atan2f(float y, float x);
    case lfn::atan2f:
    /// long double atan2l(long double y, long double x);
    case lfn::atan2l:
        return just(callFloat2(LAM2(x, y, std::atan2(x, y)), F->getReturnType(), ArgVals));

    /// double copysign(double x, double y);
    case lfn::copysign:
    /// float copysignf(float x, float y);
    case lfn::copysignf:
    /// long double copysignl(long double x, long double y);
    case lfn::copysignl:
        return just(callFloat2(LAM2(x, y, std::copysign(x, y)), F->getReturnType(), ArgVals));

    /// double pow(double x, double y);
    case lfn::pow:
    /// float powf(float x, float y);
    case lfn::powf:
    /// long double powl(long double x, long double y);
    case lfn::powl:
        return just(callFloat2(LAM2(x, y, std::pow(x, y)), F->getReturnType(), ArgVals));

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
    }

    }

    return nothing();
}

#include "Util/unmacros.h"

