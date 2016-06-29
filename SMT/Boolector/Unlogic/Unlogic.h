//
// Created by belyaev on 6/24/16.
//

#ifndef AURORA_SANDBOX_UNLOGIC_H
#define AURORA_SANDBOX_UNLOGIC_H

#include <SMT/Boolector/Boolector.h>

namespace borealis {
namespace boolector_ {
namespace unlogic {

Term::Ptr undoThat(const FactoryNest& FN, Term::Ptr witness, borealis::Boolector::Dynamic dyn);

} /* namespace  */
} /* namespace smt */
} /* namespace borealis */

#endif //AURORA_SANDBOX_UNLOGIC_H
