/*
 * util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include <llvm/Support/Program.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/LLVMContext.h>

#include <cstdlib>
#include <unordered_set>
#include <llvm/IR/CallSite.h>

#include "Util/cast.hpp"
#include "Util/functional.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace llvm {

llvm::raw_ostream& operator<<(llvm::raw_ostream& OS, const llvm::Type& T) {
    T.print(OS);
    return OS;
}

////////////////////////////////////////////////////////////////////////////////

typedef std::pair<std::string, ConditionType> ConditionDescription;

ConditionDescription analyzeCondition(const int cond) {
    typedef CmpInst::Predicate P;
    typedef ConditionType CT;

    switch (cond) {
    case P::ICMP_EQ:
    case P::FCMP_OEQ:
        return {"=", CT::EQ};
    case P::FCMP_UEQ:
        return {"?=", CT::EQ};

    case P::ICMP_NE:
    case P::FCMP_ONE:
        return {"~=", CT::NEQ};
    case P::FCMP_UNE:
        return {"?~=", CT::NEQ};

    case P::ICMP_SGE:
    case P::FCMP_OGE:
        return {">=", CT::GE};
    case P::ICMP_UGE:
        return {"+>=", CT::UGE};
    case P::FCMP_UGE:
        return {"?>=", CT::GE};

    case P::ICMP_SGT:
    case P::FCMP_OGT:
        return {">", CT::GT};
    case P::ICMP_UGT:
        return {"+>", CT::UGT};
    case P::FCMP_UGT:
        return {"?>", CT::GT};

    case P::ICMP_SLE:
    case P::FCMP_OLE:
        return {"<=", CT::LE};
    case P::ICMP_ULE:
        return {"+<=", CT::ULE};
    case P::FCMP_ULE:
        return {"?<=", CT::LE};

    case P::ICMP_SLT:
    case P::FCMP_OLT:
        return {"<", CT::LT};
    case P::ICMP_ULT:
        return {"+<", CT::ULT};
    case P::FCMP_ULT:
        return {"?<", CT::LT};

    // FIXME: correct processing for FCMP_ORD / FCMP_UNO

    case P::FCMP_TRUE:
    case P::FCMP_ORD:
        return {"true", CT::TRUE};
    case P::FCMP_FALSE:
    case P::FCMP_UNO:
        return {"false", CT::FALSE};

    default:
        BYE_BYE(ConditionDescription, "Unreachable!");
    }
}

ConditionType conditionType(int cond) {
    return analyzeCondition(cond).second;
}

std::string conditionString(int cond) {
    return analyzeCondition(cond).first;
}

std::string conditionString(ConditionType cond) {
    switch(cond) {
    case ConditionType::EQ:    return "==";
    case ConditionType::NEQ:   return "!=";

    case ConditionType::GT:    return ">";
    case ConditionType::GE:    return ">=";
    case ConditionType::LT:    return "<";
    case ConditionType::LE:    return "<=";

    case ConditionType::UGT:   return "+>";
    case ConditionType::UGE:   return "+>=";
    case ConditionType::ULT:   return "+<";
    case ConditionType::ULE:   return "+<=";

    case ConditionType::TRUE:  return "true";
    case ConditionType::FALSE: return "false";
    default: BYE_BYE(std::string, "Unreachable!");
    }
}

ConditionType forceSigned(ConditionType cond) {
    switch(cond) {
    case ConditionType::UGT: return ConditionType::GT;
    case ConditionType::UGE: return ConditionType::GE;
    case ConditionType::ULT: return ConditionType::LT;
    case ConditionType::ULE: return ConditionType::LE;
    default: return cond;
    }
}

ConditionType forceUnsigned(ConditionType cond) {
    switch(cond) {
    case ConditionType::GT: return ConditionType::UGT;
    case ConditionType::GE: return ConditionType::UGE;
    case ConditionType::LT: return ConditionType::ULT;
    case ConditionType::LE: return ConditionType::ULE;
    default: return cond;
    }
}

ConditionType makeNot(ConditionType cond) {
    switch(cond) {
    case ConditionType::EQ:    return ConditionType::NEQ;
    case ConditionType::NEQ:   return ConditionType::EQ;

    case ConditionType::GT:    return ConditionType::LE;
    case ConditionType::GE:    return ConditionType::LT;
    case ConditionType::LT:    return ConditionType::GE;
    case ConditionType::LE:    return ConditionType::GT;

    case ConditionType::UGT:   return ConditionType::ULE;
    case ConditionType::UGE:   return ConditionType::ULT;
    case ConditionType::ULT:   return ConditionType::UGE;
    case ConditionType::ULE:   return ConditionType::UGT;

    case ConditionType::TRUE:  return ConditionType::FALSE;
    case ConditionType::FALSE: return ConditionType::TRUE;
    default: BYE_BYE(ConditionType, "Unreachable!");
    }
}

////////////////////////////////////////////////////////////////////////////////

ArithType arithType(llvm::BinaryOperator::BinaryOps llops) {
    typedef llvm::BinaryOperator::BinaryOps ops;

    switch(llops) {
    case ops::Add:  return ArithType::ADD;
    case ops::FAdd: return ArithType::ADD;
    case ops::Sub:  return ArithType::SUB;
    case ops::FSub: return ArithType::SUB;
    case ops::Mul:  return ArithType::MUL;
    case ops::FMul: return ArithType::MUL;
    case ops::UDiv: return ArithType::DIV;
    case ops::SDiv: return ArithType::DIV;
    case ops::FDiv: return ArithType::DIV;
    case ops::URem: return ArithType::REM;
    case ops::SRem: return ArithType::REM;
    case ops::FRem: return ArithType::REM;

    // Logical operators (integer operands)
    case ops::Shl:  return ArithType::SHL;
    case ops::LShr: return ArithType::LSHR;
    case ops::AShr: return ArithType::ASHR;
    case ops::And:  return ArithType::BAND;
    case ops::Or:   return ArithType::BOR;
    case ops::Xor:  return ArithType::XOR;
    default: BYE_BYE(ArithType, "Unreachable!");
    }
}

std::string arithString(ArithType opCode) {
    switch (opCode) {
    case ArithType::ADD:  return "+";
    case ArithType::SUB:  return "-";
    case ArithType::MUL:  return "*";
    case ArithType::DIV:  return "/";
    case ArithType::REM:  return "%";
    case ArithType::BAND: return "&";
    case ArithType::BOR:  return "|";
    case ArithType::LAND: return "&&";
    case ArithType::LOR:  return "||";
    case ArithType::XOR:  return "^";
    case ArithType::SHL:  return "<<";
    case ArithType::ASHR: return ">>";
    case ArithType::LSHR: return ">>>";
    case ArithType::IMPLIES: return "==>";
    default: BYE_BYE(std::string, "Unreachable!");
    }
}

std::string unaryArithString(UnaryArithType opCode) {
    switch (opCode) {
    case UnaryArithType::NOT:  return "!";
    case UnaryArithType::BNOT: return "~";
    case UnaryArithType::NEG:  return "-";
    default: BYE_BYE(std::string, "Unreachable!");
    }
}

////////////////////////////////////////////////////////////////////////////////

llvm::Constant* getBoolConstant(bool b) {
    return b ? llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(llvm::getGlobalContext()))
             : llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(llvm::getGlobalContext()));
}

llvm::Constant* getIntConstant(uint64_t i) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), i);
}

llvm::ConstantPointerNull* getNullPointer() {
    return llvm::ConstantPointerNull::get(llvm::Type::getInt64PtrTy(llvm::getGlobalContext()));
}

std::list<Loop*> getAllLoops(Function* F, LoopInfo* LI) {
    std::unordered_set<Loop*> loops;

    for (const auto& BB : *F) {
        loops.insert(LI->getLoopFor(&BB));
    }
    loops.erase(nullptr);

    return std::list<Loop*>(loops.begin(), loops.end());
}

Loop* getLoopFor(Instruction* inst, LoopInfo* LI) {
    return LI->getLoopFor(inst->getParent());
}

std::list<ReturnInst*> getAllRets(Function* F) {
    using borealis::util::viewContainer;
    using namespace borealis::ops;

    std::unordered_set<ReturnInst*> rets;

    for (ReturnInst* RI : viewContainer(*F)
                          .flatten()
                          .map(take_pointer)
                          .map(dyn_caster<ReturnInst>())
                          .filter()) {
        rets.insert(RI);
    }

    return std::list<ReturnInst*>(rets.begin(), rets.end());
}

ReturnInst* getSingleRetOpt(Function* F) {
    auto rets = getAllRets(F);

    if (rets.size() == 1) {
        return rets.front();
    } else {
        return nullptr;
    }
}

bool hasAddressTaken(const llvm::Function& F) {
    if (not F.hasAddressTaken()) return false;

    for(auto&& user : F.users()) {
        if (llvm::is_one_of<llvm::CallInst, llvm::InvokeInst>(user)) {
            if (llvm::ImmutableCallSite(user).getCalledFunction() == &F) continue;
            else return true;

        } else if (auto&& ce = llvm::dyn_cast<llvm::ConstantExpr>(user)) {
            if (ce->getOpcode() != llvm::Instruction::BitCast) return true;

            for (auto&& castuser : user->users()) {
                if (not llvm::isa<llvm::CallInst>(castuser)) return true;
            }
        }
        else return true;
    }
    return false;
}

llvm::Function* getCalledFunction(const llvm::Value* v) {
    if(auto&& ii = llvm::dyn_cast<llvm::InvokeInst>(v)) {
        return ii->getCalledFunction();
    }
    if(auto&& i = llvm::dyn_cast<llvm::CallInst>(v)) {
        return i->getCalledFunction();
    }

    return nullptr;
}

} // namespace llvm

namespace borealis {
namespace util {

static std::string executableDirectory;

void initFilePaths(const char ** argv) {
    auto execPath = llvm::sys::FindProgramByName(argv[0]);
    executableDirectory = llvm::sys::path::parent_path(execPath).str();
}

static bool fileExists(llvm::Twine path) {
    llvm::sys::fs::file_status fst;
    llvm::sys::fs::status(path, fst);
    return llvm::sys::fs::exists(fst);
}

std::string getFilePathIfExists(const std::string& path) {
    if(llvm::sys::path::is_absolute(path)) {
        if(fileExists(path)) return path;
        return "";
    }

    llvm::SmallString<256> tryPath;
    llvm::sys::fs::current_path(tryPath);
    llvm::sys::path::append(tryPath, path);

    if(fileExists(llvm::Twine(tryPath))) {
        return tryPath.str().str();
    }

    tryPath = llvm::StringRef(executableDirectory);
    llvm::sys::path::append(tryPath, path);

    if(fileExists(llvm::Twine(tryPath))) {
        return tryPath.str().str();
    }

    return "";
}

std::string nospaces(const std::string& v) {
    return nospaces(std::string(v));
}

std::string nospaces(std::string&& v) {
    v.erase(std::remove_if(v.begin(), v.end(), isspace), v.end());
    return v;
}

bool endsWith(const std::string& fullString, const std::string& ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

std::string nochar(const std::string& v, char c) {
    return nochar(std::string(v), c);
}

std::string nochar(std::string&& v, char c) {
    v.erase(std::remove_if(v.begin(), v.end(), [&c](char x) { return c == x; }), v.end());
    return v;
}

std::string& replace(const std::string& from, const std::string& to, std::string& in) {
    auto pos = in.find(from);
    if (pos == std::string::npos) return in;
    else return in.replace(pos, from.length(), to);
}

} // namespace util
} // namespace borealis

#include "Util/unmacros.h"
