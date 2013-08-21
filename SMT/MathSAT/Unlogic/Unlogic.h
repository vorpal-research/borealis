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
namespace mathsat_ {
namespace unlogic {

USING_SMT_LOGIC(MathSAT)

PredicateState::Ptr undoThat(Dynamic& dyn);

} // namespace unlogic
} //namespace mathsat_
} //namespace borealis


#endif /* MATHSAT_UNLOGIC_H_ */
