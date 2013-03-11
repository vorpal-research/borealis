/*
 * util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include <llvm/Constants.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>
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
		return {">=", CT::GTE};
	case P::ICMP_UGE:
		return {"+>=", CT::GTE};
	case P::FCMP_UGE:
		return {"?>=", CT::GTE};

	case P::ICMP_SGT:
	case P::FCMP_OGT:
		return {">", CT::GT};
	case P::ICMP_UGT:
		return {"+>", CT::GT};
	case P::FCMP_UGT:
		return {"?>", CT::GT};

	case P::ICMP_SLE:
	case P::FCMP_OLE:
		return {"<=", CT::LTE};
	case P::ICMP_ULE:
		return {"+<=", CT::LTE};
	case P::FCMP_ULE:
		return {"?<=", CT::LTE};

	case P::ICMP_SLT:
	case P::FCMP_OLT:
		return {"<", CT::LT};
	case P::ICMP_ULT:
		return {"+<", CT::LT};
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

std::string conditionString(int cond) {
	return analyzeCondition(cond).first;
}

std::string conditionString(ConditionType cond) {
    switch(cond) {
    case ConditionType::EQ: return "==";
    case ConditionType::NEQ: return "!=";
    case ConditionType::GT: return ">";
    case ConditionType::GTE: return ">=";
    case ConditionType::LT: return "<";
    case ConditionType::LTE: return "<=";
    case ConditionType::TRUE: return "true";
    case ConditionType::FALSE: return "false";
    default: BYE_BYE(std::string, "Unreachable!");
    }
}

ConditionType conditionType(int cond) {
	return analyzeCondition(cond).second;
}

ValueType valueType(const llvm::Value& value) {
	return type2type(*value.getType(),
	        isa<llvm::ConstantPointerNull>(value) ? TypeInfo::CONSTANT_POINTER_NULL :
	        isa<llvm::Constant>(value) ? TypeInfo::CONSTANT :
	        TypeInfo::VARIABLE);
}

ValueType type2type(const llvm::Type& type, TypeInfo info) {
    using namespace::llvm;

    typedef ValueType VT;

    switch(info) {
    case TypeInfo::VARIABLE:
        {
            if (type.isIntegerTy()) {
                if (type.getPrimitiveSizeInBits() == 1) {
                    return VT::BOOL_VAR;
                } else {
                    return VT::INT_VAR;
                }
            } else if (type.isFloatingPointTy()) {
                return VT::REAL_VAR;
            } else if (type.isPointerTy()) {
                return VT::PTR_VAR;
            } else {
                return VT::UNKNOWN;
            }
        }
    case TypeInfo::CONSTANT:
        {
            if (type.isIntegerTy()) {
                if (type.getPrimitiveSizeInBits() == 1) {
                    return VT::BOOL_CONST;
                } else {
                    return VT::INT_CONST;
                }
            } else if (type.isFloatingPointTy()) {
                return VT::REAL_CONST;
            } else if (type.isPointerTy()) {
                return VT::PTR_CONST;
            } else {
                return VT::UNKNOWN;
            }
        }
    case TypeInfo::CONSTANT_POINTER_NULL:
        {
            return VT::NULL_PTR_CONST;
        }
    default:
        BYE_BYE(ValueType, "Unreachable!");
    }
}

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
    case ops::Shl:  return ArithType::LSH;
    case ops::LShr: return ArithType::RSH;
    case ops::AShr: return ArithType::RSH;
    case ops::And:  return ArithType::BAND;
    case ops::Or:   return ArithType::BOR;
    case ops::Xor:  return ArithType::XOR;
    default: BYE_BYE(ArithType, "Unreachable");
    }
}

std::string arithString(ArithType opCode) {
    switch (opCode) {
    case ArithType::ADD: return "+";
    case ArithType::SUB: return "-";
    case ArithType::MUL: return "*";
    case ArithType::DIV: return "/";
    case ArithType::REM: return "%";
    case ArithType::BAND: return "&";
    case ArithType::BOR: return "|";
    case ArithType::LAND: return "&&";
    case ArithType::LOR: return "||";
    case ArithType::XOR: return "^";
    case ArithType::LSH: return "<<";
    case ArithType::RSH: return ">>";
    default: BYE_BYE(std::string, "Unreachable!");
    }
}

std::string unaryArithString(UnaryArithType opCode) {
    switch (opCode) {
    case UnaryArithType::NOT: return "!";
    case UnaryArithType::BNOT: return "~";
    case UnaryArithType::NEG: return "-";
    default: BYE_BYE(std::string, "Unreachable!");
    }
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
