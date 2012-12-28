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

#include "util.h"

#include "macros.h"

namespace llvm {

// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(
		llvm::raw_ostream& ost,
		llvm::raw_ostream& (*op)(llvm::raw_ostream&)) {
	return op(ost);
}

std::pair<std::string, ConditionType> analyzeCondition(const int cond) {
	using namespace::std;

	typedef CmpInst::Predicate P;
	typedef ConditionType CT;

	switch (cond) {
	case P::ICMP_EQ:
	case P::FCMP_OEQ:
		return make_pair("=", CT::EQ);
	case P::FCMP_UEQ:
		return make_pair("?=", CT::EQ);

	case P::ICMP_NE:
	case P::FCMP_ONE:
		return make_pair("~=", CT::NEQ);
	case P::FCMP_UNE:
		return make_pair("?~=", CT::NEQ);

	case P::ICMP_SGE:
	case P::FCMP_OGE:
		return make_pair(">=", CT::GTE);
	case P::ICMP_UGE:
		return make_pair("+>=", CT::GTE);
	case P::FCMP_UGE:
		return make_pair("?>=", CT::GTE);

	case P::ICMP_SGT:
	case P::FCMP_OGT:
		return make_pair(">", CT::GT);
	case P::ICMP_UGT:
		return make_pair("+>", CT::GT);
	case P::FCMP_UGT:
		return make_pair("?>", CT::GT);

	case P::ICMP_SLE:
	case P::FCMP_OLE:
		return make_pair("<=", CT::LTE);
	case P::ICMP_ULE:
		return make_pair("+<=", CT::LTE);
	case P::FCMP_ULE:
		return make_pair("?<=", CT::LTE);

	case P::ICMP_SLT:
	case P::FCMP_OLT:
		return make_pair("<", CT::LT);
	case P::ICMP_ULT:
		return make_pair("+<", CT::LT);
	case P::FCMP_ULT:
		return make_pair("?<", CT::LT);

	case P::FCMP_TRUE:
		return make_pair("true", CT::TRUE);
	case P::FCMP_FALSE:
		return make_pair("false", CT::FALSE);

	default:
		return make_pair("???", CT::UNKNOWN);
	}
}

std::string conditionString(const int cond) {
	return analyzeCondition(cond).first;
}

ConditionType conditionType(const int cond) {
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
        return VT::UNKNOWN;
    }
}

llvm::Constant* getBoolConstant(bool b) {
    return b ? llvm::ConstantInt::getTrue(llvm::Type::getInt1Ty(llvm::getGlobalContext()))
             : llvm::ConstantInt::getFalse(llvm::Type::getInt1Ty(llvm::getGlobalContext()));
}

llvm::Constant* getIntConstant(uint64_t i) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm::getGlobalContext()), i);
}

std::list<Loop*> getAllLoops(Function* F, LoopInfo* LI) {
    std::unordered_set<Loop*> loops;

    for (const auto& BB : *F) {
        loops.insert(LI->getLoopFor(&BB));
    }
    loops.erase(nullptr);

    return std::list<Loop*>(loops.begin(), loops.end());
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

bool endsWith(std::string const &fullString, std::string const &ending) {
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

#include "unmacros.h"
