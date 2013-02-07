/*
 * util.h
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Value.h>

#include <list>
#include <string>

#include "Util/util.hpp"

#include "Util/collections.hpp"
#include "Util/enums.hpp"
#include "Util/hash.hpp"
#include "Util/meta.hpp"
#include "Util/option.hpp"
#include "Util/streams.hpp"
#include "Util/locations.h"

namespace llvm {

// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(
		llvm::raw_ostream& ost,
		llvm::raw_ostream& (*op)(llvm::raw_ostream&));

llvm::raw_ostream& operator<<(llvm::raw_ostream& OS, const llvm::Type& T);

enum class ConditionType {
	EQ,
	NEQ,
	GT,
	GTE,
	LT,
	LTE,
	TRUE,
	FALSE,
	UNKNOWN
};
std::string conditionString(const int cond);
std::string conditionString(ConditionType cond);
ConditionType conditionType(const int cond);

enum class TypeInfo {
    VARIABLE,
    CONSTANT,
    CONSTANT_POINTER_NULL
};
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
ValueType type2type(const llvm::Type& type, TypeInfo info = TypeInfo::VARIABLE);

llvm::Constant* getBoolConstant(bool b);
llvm::Constant* getIntConstant(uint64_t i);

std::list<Loop*> getAllLoops(Function* F, LoopInfo* LI);
Loop* getLoopFor(Instruction* Inst, LoopInfo* LI);

enum class ArithType {
    ADD,
    SUB,
    MUL,
    DIV,
    REM,
    LAND,
    LOR,
    BAND,
    BOR,
    XOR,
    LSH,
    RSH
};
std::string arithString(ArithType opCode);

enum class UnaryArithType {
    NEG,
    NOT,
    BNOT,
};
std::string unaryArithString(UnaryArithType opCode);

inline borealis::Locus instructionLocus(const Instruction* inst) {
    auto node = inst->getMetadata("dbg");
    if(node) return DILocation(node);
    else return borealis::Locus();
}

} // namespace llvm

namespace borealis {
namespace util {

std::string nospaces(const std::string& v);
std::string nospaces(std::string&& v);
bool endsWith(const std::string& fullString, const std::string& ending);

namespace streams {

// copy the standard ostream endl
llvm::raw_ostream& endl(llvm::raw_ostream& ost);

} // namespace streams
} // namespace util
} // namespace borealis

// FIXME: maybe this thing can be made less explicit???
namespace std {
////////////////////////////////////////////////////////////////////////////////
// ConditionType
////////////////////////////////////////////////////////////////////////////////
template<>
struct hash<llvm::ConditionType> {
    size_t operator()(const llvm::ConditionType& e) {
        return borealis::util::enums::enum_hash<llvm::ConditionType>()(e);
    }
};
template<>
struct hash<const llvm::ConditionType> {
    size_t operator()(const llvm::ConditionType& e) {
        return borealis::util::enums::enum_hash<llvm::ConditionType>()(e);
    }
};
////////////////////////////////////////////////////////////////////////////////
// ValueType
////////////////////////////////////////////////////////////////////////////////
template<>
struct hash<llvm::ValueType> {
    size_t operator()(const llvm::ValueType& e) {
        return borealis::util::enums::enum_hash<llvm::ValueType>()(e);
    }
};
template<>
struct hash<const llvm::ValueType> {
    size_t operator()(const llvm::ValueType& e) {
        return borealis::util::enums::enum_hash<llvm::ValueType>()(e);
    }
};
////////////////////////////////////////////////////////////////////////////////
// ArithType
////////////////////////////////////////////////////////////////////////////////
template<>
struct hash<llvm::ArithType> {
    size_t operator()(const llvm::ArithType& e) {
        return borealis::util::enums::enum_hash<llvm::ArithType>()(e);
    }
};
template<>
struct hash<const llvm::ArithType> {
    size_t operator()(const llvm::ArithType& e) {
        return borealis::util::enums::enum_hash<llvm::ArithType>()(e);
    }
};
////////////////////////////////////////////////////////////////////////////////
// UnaryArithType
////////////////////////////////////////////////////////////////////////////////
template<>
struct hash<llvm::UnaryArithType> {
    size_t operator()(const llvm::UnaryArithType& e) {
        return borealis::util::enums::enum_hash<llvm::UnaryArithType>()(e);
    }
};
template<>
struct hash<const llvm::UnaryArithType> {
    size_t operator()(const llvm::UnaryArithType& e) {
        return borealis::util::enums::enum_hash<llvm::UnaryArithType>()(e);
    }
};
}

#endif /* UTIL_H_ */
