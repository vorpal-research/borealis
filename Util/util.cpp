/*
 * util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include <llvm/Constants.h>
#include <llvm/InstrTypes.h>
#include <llvm/LLVMContext.h>

#include <cstdlib>
#include <unordered_set>

#include "Util/util.h"

#include "Util/macros.h"

namespace llvm {

// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(
		llvm::raw_ostream& ost,
		llvm::raw_ostream& (*op)(llvm::raw_ostream&)) {
	return op(ost);
}

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

	case P::FCMP_TRUE:
		return {"true", CT::TRUE};
	case P::FCMP_FALSE:
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
    using borealis::util::isValid;
    using borealis::util::takePtr;
    using borealis::util::viewContainer;

    std::unordered_set<ReturnInst*> rets;

    for (ReturnInst* RI : viewContainer(F).flatten()
                          .map(takePtr())
                          .map(dyn_caster<ReturnInst>())
                          .filter(isValid())) {
        rets.insert(RI);
    }

    return std::list<ReturnInst*>(rets.begin(), rets.end());
}

} // namespace llvm

namespace borealis {
namespace util {

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

namespace streams {

// copy the standard ostream endl
llvm::raw_ostream& endl(llvm::raw_ostream& ost) {
	ost << '\n';
	ost.flush();
	return ost;
}

} // namespace streams
} // namespace util
} // namespace borealis

#include "Util/unmacros.h"
