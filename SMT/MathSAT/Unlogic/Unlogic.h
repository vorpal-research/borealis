/*
 * Unlogic.h
 *
 *  Created on: Aug 19, 2013
 *      Author: sam
 */

#ifndef MATHSAT_UNLOGIC_H_
#define MATHSAT_UNLOGIC_H_

#include "SMT/MathSAT/MathSAT.h"
#include "State/PredicateState.h"

namespace borealis {
namespace mathsat {

PredicateState::Ptr undoThat(Expr& expr);

} //namespace mathsat
} //namespace borealis


#endif /* MATHSAT_UNLOGIC_H_ */
