//
// Created by abdullin on 2/11/19.
//

#include "AbstractDomain.hpp"

#include "Util/sayonara.hpp"
#include "Util/macros.h"

namespace borealis {
namespace absint {

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

AbstractDomain::Ptr AbstractDomain::apply(BinaryOperator, AbstractDomain::ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::apply(CmpOperator, AbstractDomain::ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::load(Type::Ptr, Ptr) const {
    UNREACHABLE("Unsupported operation");
}

void AbstractDomain::store(Ptr, Ptr) {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::gep(Type::Ptr, const std::vector<AbstractDomain::Ptr>&) {
    UNREACHABLE("Unsupported operation");
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"
