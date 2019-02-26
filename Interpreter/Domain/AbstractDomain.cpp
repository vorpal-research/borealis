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

AbstractDomain::Ptr AbstractDomain::apply(llvm::ArithType, AbstractDomain::ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

AbstractDomain::Ptr AbstractDomain::apply(llvm::ConditionType, AbstractDomain::ConstPtr) const {
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

Split AbstractDomain::splitByEq(ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

Split AbstractDomain::splitByLess(ConstPtr) const {
    UNREACHABLE("Unsupported operation");
}

std::ostream& operator<<(std::ostream& s, AbstractDomain::Ptr domain) {
    s << domain->toString();
    return s;
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, AbstractDomain::Ptr domain) {
    s << domain->toString();
    return s;
}

} // namespace absint
} // namespace borealis

#include "Util/unmacros.h"
