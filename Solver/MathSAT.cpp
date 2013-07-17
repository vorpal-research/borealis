/*
 * MathSAT.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: Sam Kolton
 */

#include "MathSAT.h"
#include "Util/util.h"
#include "Util/macros.h"

namespace mathsat {

Decl Expr::decl() const {
	msat_decl ret = msat_term_get_decl(term_);
	bool ass = MSAT_ERROR_DECL(ret);
	ASSERTC(!ass)
	return ret;
}

Sort Expr::arg_type(unsigned i) const {
	if (num_args() < i) {
		return nullptr;
	}
	auto type = msat_decl_get_arg_type(decl(), i);
	return type;
}

} // namespace mathsat



