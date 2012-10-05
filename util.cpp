/*
 * util.cpp
 *
 *  Created on: Aug 22, 2012
 *      Author: belyaev
 */

#include "util.h"

#include <llvm/InstrTypes.h>

namespace llvm {
// copy the standard ostream behavior with functions
llvm::raw_ostream& operator<<(
		llvm::raw_ostream& ost,
		llvm::raw_ostream& (*op)(llvm::raw_ostream&)) {
	return op(ost);
}

std::string conditionToString(const int cond) {
	typedef CmpInst::Predicate P;

	switch (cond) {
	case P::ICMP_EQ:
	case P::FCMP_OEQ:
		return "=";
	case P::FCMP_UEQ:
		return "?=";

	case P::ICMP_NE:
	case P::FCMP_ONE:
		return "~=";
	case P::FCMP_UNE:
		return "?~=";

	case P::ICMP_SGE:
	case P::FCMP_OGE:
		return ">=";
	case P::ICMP_UGE:
		return "+>=";
	case P::FCMP_UGE:
		return "?>=";

	case P::ICMP_SGT:
	case P::FCMP_OGT:
		return ">";
	case P::ICMP_UGT:
		return "+>";
	case P::FCMP_UGT:
		return "?>";

	case P::ICMP_SLE:
	case P::FCMP_OLE:
		return "<=";
	case P::ICMP_ULE:
		return "+<=";
	case P::FCMP_ULE:
		return "?<=";

	case P::ICMP_SLT:
	case P::FCMP_OLT:
		return "<";
	case P::ICMP_ULT:
		return "+<";
	case P::FCMP_ULT:
		return "?<";

	case P::FCMP_TRUE:
		return "true";
	case P::FCMP_FALSE:
		return "false";

	default:
		return "???";
	}
}
} // namespace llvm

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
