//
// Created by abdullin on 7/14/17.
//

#include "DomainFactory.h"
#include "FunctionDomain.h"
#include "Util/collections.hpp"

#include "Util/macros.h"

namespace borealis {
namespace absint {


FunctionDomain::FunctionDomain(DomainFactory* factory, const llvm::Type* type)
        : Domain(BOTTOM, FUNCTION, factory),
          type_(type) {}

FunctionDomain::FunctionDomain(DomainFactory* factory, const llvm::Type* type, Function::Ptr location)
        : Domain(VALUE, FUNCTION, factory),
          type_(type) {
    locations_.insert(location);
}

FunctionDomain::FunctionDomain(DomainFactory* factory,
                                 const llvm::Type* type,
                                 const FunctionSet& locations)
        : Domain(VALUE, FUNCTION, factory),
          type_(type),
          locations_(locations) {}

bool FunctionDomain::equals(const Domain* other) const {
    auto func = llvm::dyn_cast<FunctionDomain>(other);
    if (not func) return false;
    if (this == func) return true;

    if (locations_.size() != func->locations_.size()) return false;

    return util::equal_with_find(locations_, func->locations_,
                                 [](auto&& a) { return a; },
                                 [](auto&& a, auto&& b) { return a->equals(b.get()); });
}

bool FunctionDomain::operator<(const Domain&) const {
    UNREACHABLE("Unimplemented function");
}

Domain::Ptr FunctionDomain::join(Domain::Ptr other) {
    auto func = llvm::dyn_cast<FunctionDomain>(other.get());
    ASSERT(func, "Non-function domain in function join");

    for (auto&& it : func->getLocations()) {
        if (not util::contains(locations_, it)) {
            locations_.insert(it);
        }
    }
    return shared_from_this();
}

const FunctionDomain::FunctionSet& FunctionDomain::getLocations() const {
    return locations_;
}

Domain::Ptr FunctionDomain::meet(Domain::Ptr) {
    UNREACHABLE("Unimplemented function");
}

Domain::Ptr FunctionDomain::widen(Domain::Ptr other) {
    return join(other);
}

std::size_t FunctionDomain::hashCode() const {
    return locations_.size();
}

std::string FunctionDomain::toPrettyString(const std::string& prefix) const {
    std::ostringstream ss;
    ss << "Function " << factory_->getSlotTracker().toString(type_) << " : ";
    for (auto&& it : locations_) {
        ss << std::endl << prefix << "  " << it->getName();
    }
    ss << std::endl << prefix << "]";
    return ss.str();
}

bool FunctionDomain::classof(const Domain* other) {
    return other->getType() == Domain::FUNCTION;
}

void FunctionDomain::moveToTop() {
    setTop();
    locations_.clear();
}

}   // namespace absint
}   // namespace borealis
