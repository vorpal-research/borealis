//
// Created by abdullin on 3/22/19.
//

#ifndef BOREALIS_OCTAGONDOMAIN_HPP
#define BOREALIS_OCTAGONDOMAIN_HPP

#include "Interpreter/Domain/Numerical/Apron/ApronDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

template <typename Number, typename Variable, typename VarHash, typename VarEquals>
using OctagonDomain = apron::ApronDomain<Number, Variable, VarHash, VarEquals, apron::DomainT::OCTAGON>;

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"

#endif //BOREALIS_OCTAGONDOMAIN_HPP
