#define DEBUG_TYPE "uSSA"

#include "PtrSSAPass.h"
#include "llvm/Support/CommandLine.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/Casting.h>
using llvm::ArrayRef;

#include <regex>

using namespace llvm;
using namespace borealis;

#include "intrinsics.h"
#include "PtrSSAPass/SLInjectionPass.h"
#include "PtrSSAPass/PhiInjectionPass.h"


char borealis::PtrSSAPass::ID;
static RegisterPass<PtrSSAPass> X("pointer-ssa",
        "The pass that places an intrinsic mark on every pointer before every use");
