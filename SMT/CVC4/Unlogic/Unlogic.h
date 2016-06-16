/*
 * Unlogic.h
 *
 *  Created on: Feb 11, 2014
 *      Author: sam
 */

#ifndef CVC4_UNLOGIC_H_
#define CVC4_UNLOGIC_H_

#include "SMT/CVC4/CVC4.h"
#include "Term/Term.h"

namespace borealis {
namespace cvc4_ {
namespace unlogic {

Term::Ptr undoThat(borealis::CVC4::Dynamic dyn);

} // namespace unlogic
} // namespace z3_
} // namespace borealis

#endif /* CVC4_UNLOGIC_H_ */
