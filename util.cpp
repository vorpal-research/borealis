/*
 * util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include <llvm/Constants.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

#include <cstdlib>

#include "util.h"

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
		return make_pair("???", CT::WTF);
	}
}

std::string conditionString(const int cond) {
	return analyzeCondition(cond).first;
}

ConditionType conditionType(const int cond) {
	return analyzeCondition(cond).second;
}

ValueType valueType(const llvm::Value& value) {
	using namespace::llvm;

	typedef ValueType VT;

	Type* type = value.getType();
	if (isa<Constant>(value)) {
		if (type->isIntegerTy()) {
			return VT::INT_CONST;
		} else if (type->isFloatingPointTy()) {
			return VT::REAL_CONST;
		} else if (isa<ConstantPointerNull>(value)) {
			return VT::NULL_PTR_CONST;
		} else if (type->isPointerTy()) {
			return VT::PTR_CONST;
		} else {
			return VT::UNKNOWN;
		}
	} else {
		if (type->isIntegerTy()) {
			return VT::INT_VAR;
		} else if (type->isFloatingPointTy()) {
			return VT::REAL_VAR;
		} else if (type->isPointerTy()) {
			return VT::PTR_VAR;
		} else {
			return VT::UNKNOWN;
		}
	}
}

} // namespace llvm

namespace borealis {
namespace util {

std::string nospaces(const std::string& v) {
	return nospaces(std::string(v));
}

using std::remove_if;

std::string nospaces(std::string&& v) {
	v.erase(remove_if(v.begin(), v.end(), isspace), v.end());
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
