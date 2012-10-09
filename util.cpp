/*
 * util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include "util.h"

#include <llvm/Constants.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

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

	switch (cond) {
	case P::ICMP_EQ:
	case P::FCMP_OEQ:
		return make_pair("=", EQ);
	case P::FCMP_UEQ:
		return make_pair("?=", EQ);

	case P::ICMP_NE:
	case P::FCMP_ONE:
		return make_pair("~=", NEQ);
	case P::FCMP_UNE:
		return make_pair("?~=", NEQ);

	case P::ICMP_SGE:
	case P::FCMP_OGE:
		return make_pair(">=", GTE);
	case P::ICMP_UGE:
		return make_pair("+>=", GTE);
	case P::FCMP_UGE:
		return make_pair("?>=", GTE);

	case P::ICMP_SGT:
	case P::FCMP_OGT:
		return make_pair(">", GT);
	case P::ICMP_UGT:
		return make_pair("+>", GT);
	case P::FCMP_UGT:
		return make_pair("?>", GT);

	case P::ICMP_SLE:
	case P::FCMP_OLE:
		return make_pair("<=", LTE);
	case P::ICMP_ULE:
		return make_pair("+<=", LTE);
	case P::FCMP_ULE:
		return make_pair("?<=", LTE);

	case P::ICMP_SLT:
	case P::FCMP_OLT:
		return make_pair("<", LT);
	case P::ICMP_ULT:
		return make_pair("+<", LT);
	case P::FCMP_ULT:
		return make_pair("?<", LT);

	case P::FCMP_TRUE:
		return make_pair("true", TRUE);
	case P::FCMP_FALSE:
		return make_pair("false", FALSE);

	default:
		return make_pair("???", WTF);
	}
}

std::string conditionToString(const int cond) {
	return analyzeCondition(cond).first;
}

ConditionType conditionToType(const int cond) {
	return analyzeCondition(cond).second;
}

ValueType valueType(const llvm::Value& value) {
	using namespace::llvm;

	Type* type = value.getType();
	if (isa<Constant>(value)) {
		if (type->isIntegerTy()) {
			return INT_CONST;
		} else if (type->isFloatingPointTy()) {
			return REAL_CONST;
		} else if (isa<ConstantPointerNull>(value)) {
			return NULL_PTR_CONST;
		} else if (type->isPointerTy()) {
			return PTR_CONST;
		} else {
			return UNKNOWN;
		}
	} else {
		if (type->isIntegerTy()) {
			return INT_VAR;
		} else if (type->isFloatingPointTy()) {
			return REAL_VAR;
		} else if (type->isPointerTy()) {
			return PTR_VAR;
		} else {
			return UNKNOWN;
		}
	}
}

ValueType derefValueType(const llvm::Value& value) {
	using namespace::llvm;

	Type* type = value.getType()->getPointerElementType();
	if (type->isIntegerTy()) {
		return INT_VAR;
	} else if (type->isFloatingPointTy()) {
		return REAL_VAR;
	} else if (type->isPointerTy()) {
		return PTR_VAR;
	} else {
		return UNKNOWN;
	}
}

} // namespace llvm

namespace z3 {
z3::expr valueByTypeToExpr(
		z3::context& ctx,
		const llvm::ValueType& vt,
		const std::string& valueName) {
	using namespace::llvm;

	switch(vt) {
	case INT_CONST:
		return ctx.int_val(valueName.c_str());
	case INT_VAR:
		return ctx.int_const(valueName.c_str());
	case REAL_CONST:
		return ctx.real_val(valueName.c_str());
	case REAL_VAR:
		return ctx.real_const(valueName.c_str());
	case BOOL_CONST:
		return ctx.bool_val(valueName.c_str());
	case BOOL_VAR:
		return ctx.bool_const(valueName.c_str());
	case NULL_PTR_CONST:
		return ctx.int_val(0);
	case PTR_CONST:
		return ctx.int_val(valueName.c_str());
	case PTR_VAR:
		return ctx.int_const(valueName.c_str());
	case UNKNOWN:
		return *((z3::expr*)0);
	}
}

z3::expr valueToExpr(
		z3::context& ctx,
		const llvm::Value& value,
		const std::string& valueName) {
	using namespace::llvm;

	ValueType vt = valueType(value);
	return valueByTypeToExpr(ctx, vt, valueName);
}

z3::expr derefValueToExpr(
		z3::context& ctx,
		const llvm::Value& value,
		const std::string& valueName) {
	using namespace::llvm;

	ValueType vt = derefValueType(value);
	return valueByTypeToExpr(ctx, vt, valueName);
}
} // namespace z3

namespace borealis {
namespace util {
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
