/*
 * FunctionCalls.cpp
 *
 *  Created on: Feb 5, 2015
 *      Author: belyaev
 */
#include "Executor/Executor.h"

using namespace borealis;

#include <Logging/tracer.hpp>
#include <Codegen/intrinsics.h>

#include <llvm/Target/TargetLibraryInfo.h>

namespace lfn = llvm::LibFunc;

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

llvm::GenericValue Executor::executeMalloc(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC

    // FIXME: different versions of malloc have different parameters, check

    llvm::GenericValue RetVal{};
    auto sz = TD->getTypeStoreSize(f->getReturnType()->getPointerElementType());
    RetVal.PointerVal = Mem.MallocMemory(sz * ArgVals[0].IntVal.getLimitedValue(), MemorySimulator::MallocFill::UNINIT);
    return RetVal;
}
llvm::GenericValue Executor::executeCalloc(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC

    llvm::GenericValue RetVal{};
    auto sz = TD->getTypeStoreSize(f->getReturnType()->getPointerElementType());
    RetVal.PointerVal = Mem.MallocMemory(sz * ArgVals[0].IntVal.getLimitedValue(), MemorySimulator::MallocFill::ZERO);
    return RetVal;
}
llvm::GenericValue Executor::executeRealloc(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    return executeMalloc(f, ArgVals);
}
llvm::GenericValue Executor::executeFree(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    // TODO
    return {};
}
llvm::GenericValue Executor::executeMemcpy(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    // TODO
    return {};
}
llvm::GenericValue Executor::executeMemset(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;

    return {};
}
llvm::GenericValue Executor::executeMemmove(const llvm::Function* f, const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC
    return executeMemcpy(f, ArgVals);
}



llvm::GenericValue Executor::callExternalFunction(
    llvm::Function *F,
    const std::vector<llvm::GenericValue> &ArgVals) {
    TRACE_FUNC;
    // TODO
#define CHECK_AND_EXEC(NAME) if(is ## NAME(F, TLI)) return execute ## NAME (F, ArgVals);
    CHECK_AND_EXEC(Malloc)
    CHECK_AND_EXEC(Calloc)
    CHECK_AND_EXEC(Realloc)
    CHECK_AND_EXEC(Free)
    CHECK_AND_EXEC(Memcpy)
    CHECK_AND_EXEC(Memset)
    CHECK_AND_EXEC(Memmove)
#undef CHECK_AND_EXEC

    if(IM->getIntrinsicType(F) == function_type::INTRINSIC_MALLOC) {
        std::vector<llvm::GenericValue> adjustedArgs;
        adjustedArgs.push_back(ArgVals[2]);
        return executeMalloc(F, adjustedArgs);
    }

    llvm::GenericValue RetVal;

    if(IM->getIntrinsicType(F) == function_type::INTRINSIC_ALLOC) {
        RetVal.PointerVal = Mem.AllocateMemory(ArgVals[2].IntVal.getLimitedValue());
        return RetVal;
    }
    switch(F->getIntrinsicID()) {
    case llvm::Intrinsic::sqrt:
        RetVal.IntVal = ArgVals[0].IntVal.sqrt();
        return RetVal;
    }

    return llvm::GenericValue{};
}


