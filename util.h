/*
 * util.h
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <llvm/Support/raw_ostream.h>
#include <llvm/Value.h>

#include <z3/z3++.h>

#include <string>



namespace llvm {
// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(
		llvm::raw_ostream& ost,
		llvm::raw_ostream& (*op)(llvm::raw_ostream&));

enum class ConditionType {
	EQ,
	NEQ,
	GT,
	GTE,
	LT,
	LTE,
	TRUE,
	FALSE,
	WTF
};

std::string conditionToString(const int cond);
ConditionType conditionToType(const int cond);

enum class ValueType {
	INT_CONST,
	INT_VAR,
	BOOL_CONST,
	BOOL_VAR,
	REAL_CONST,
	REAL_VAR,
	NULL_PTR_CONST,
	PTR_CONST,
	PTR_VAR,
	UNKNOWN
};

ValueType valueType(const llvm::Value& value);
ValueType derefValueType(const llvm::Value& value);
} // namespace llvm

namespace z3 {
z3::expr valueToExpr(
		z3::context& ctx,
		const llvm::Value& value,
		const std::string& valueName);
z3::expr derefValueToExpr(
		z3::context& ctx,
		const llvm::Value& value,
		const std::string& valueName);
} // namespace z3

namespace borealis {
namespace util {

std::string nospaces(const std::string& v);
std::string nospaces(std::string&& v);
bool endsWith (std::string const &fullString, std::string const &ending);


namespace streams {

// copy the standard ostream endl
llvm::raw_ostream& endl(llvm::raw_ostream& ost);

} // namespace streams
} // namespace util
} // namespace borealis

#include "util.hpp"

#endif /* UTIL_H_ */
