/*
 * Unlogic.h
 *
 *  Created on: Feb 11, 2014
 *      Author: sam
 */

#ifndef Z3_UNLOGIC_H_
#define Z3_UNLOGIC_H_

#include "SMT/Z3/Z3.h"
#include "Term/Term.h"

namespace borealis {
namespace z3_ {
namespace unlogic {

Term::Ptr undoThat(borealis::Z3::Dynamic dyn);

} // namespace unlogic
} // namespace z3_
} // namespace borealis

#endif /* Z3_UNLOGIC_H_ */
