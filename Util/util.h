/*
 * util.h
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Value.h>

#include <list>
#include <string>

#include "Util/collections.hpp"
#include "Util/enums.hpp"
#include "Util/hash.hpp"
#include "Util/iterators.hpp"
#include "Util/locations.h"
#include "Util/meta.hpp"
#include "Util/option.hpp"
#include "Util/streams.hpp"
#include "Util/util.hpp"

namespace llvm {

// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(
		llvm::raw_ostream& ost,
		llvm::raw_ostream& (*op)(llvm::raw_ostream&));

llvm::raw_ostream& operator<<(llvm::raw_ostream& OS, const llvm::Type& T);

////////////////////////////////////////////////////////////////////////////////

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
ConditionType conditionType(int cond);
std::string conditionString(int cond);
std::string conditionString(ConditionType cond);

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
    SHL,
    ASHR,
    LSHR
};
ArithType arithType(llvm::BinaryOperator::BinaryOps llops);
std::string arithString(ArithType opCode);

enum class UnaryArithType {
    NEG,
    NOT,
    BNOT,
};
std::string unaryArithString(UnaryArithType opCode);

////////////////////////////////////////////////////////////////////////////////

llvm::Constant* getBoolConstant(bool b);
llvm::Constant* getIntConstant(uint64_t i);
llvm::ConstantPointerNull* getNullPointer();

std::list<Loop*> getAllLoops(Function* F, LoopInfo* LI);
Loop* getLoopFor(Instruction* Inst, LoopInfo* LI);

////////////////////////////////////////////////////////////////////////////////

inline borealis::Locus instructionLocus(const Instruction* inst) {
    auto node = inst->getMetadata("dbg");
    if(node) return DILocation(node);
    else return borealis::Locus();
}

inline std::string valueSummary(const Value* v) {
    if(!v) {
        return "<nullptr>";
    } else if(auto* f = llvm::dyn_cast<Function>(v)) {
        return ("function " + f->getName()).str();
    } else if(auto* bb = llvm::dyn_cast<BasicBlock>(v)) {
        return ("basic block " + bb->getName()).str();
    } else if(auto* i = llvm::dyn_cast<Instruction>(v)) {
        return borealis::util::toString(*i).c_str()+2;
    } else return borealis::util::toString(*v);
}

inline std::string valueSummary(const Value& v) {
    return valueSummary(&v);
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

namespace std {
////////////////////////////////////////////////////////////////////////////////
// ConditionType
////////////////////////////////////////////////////////////////////////////////
template<> struct hash<llvm::ConditionType> : borealis::util::enums::enum_hash<llvm::ConditionType> {};
template<> struct hash<const llvm::ConditionType> : borealis::util::enums::enum_hash<llvm::ConditionType> {};
////////////////////////////////////////////////////////////////////////////////
// ArithType
////////////////////////////////////////////////////////////////////////////////
template<> struct hash<llvm::ArithType> : borealis::util::enums::enum_hash<llvm::ArithType> {};
template<> struct hash<const llvm::ArithType> : borealis::util::enums::enum_hash<llvm::ArithType> {};
////////////////////////////////////////////////////////////////////////////////
// UnaryArithType
////////////////////////////////////////////////////////////////////////////////
template<> struct hash<llvm::UnaryArithType> : borealis::util::enums::enum_hash<llvm::UnaryArithType> {};
template<> struct hash<const llvm::UnaryArithType> : borealis::util::enums::enum_hash<llvm::UnaryArithType> {};
}

#endif /* UTIL_H_ */
