//
// Created by belyaev on 6/24/16.
//

#ifndef STP_UNLOGIC_H
#define STP_UNLOGIC_H

#include "SMT/STP/STP.h"

namespace borealis {
namespace stp_ {
namespace unlogic {

Term::Ptr undoThat(const FactoryNest& FN, Term::Ptr witness, borealis::STP::Dynamic dyn);

} /* namespace  */
} /* namespace smt */
} /* namespace borealis */

#endif //STP_UNLOGIC_H
