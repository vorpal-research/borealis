/*
 * Predicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Annotation/AssertAnnotation.h"
#include "Annotation/AssumeAnnotation.h"
#include "Annotation/EnsuresAnnotation.h"
#include "Annotation/GlobalAnnotation.h"
#include "Annotation/RequiresAnnotation.h"
#include "Predicate/Predicate.h"

#include "Util/macros.h"

namespace borealis {

PredicateType predicateType(const Annotation* a) {
   using namespace llvm;

   if (isa<RequiresAnnotation>(a)) return PredicateType::REQUIRES;
   if (isa<EnsuresAnnotation>(a)) return PredicateType::ENSURES;
   if (isa<GlobalAnnotation>(a)) return PredicateType::ASSUME;
   if (isa<AssertAnnotation>(a)) return PredicateType::ASSERT;
   if (isa<AssumeAnnotation>(a)) return PredicateType::ASSUME;
   return PredicateType::STATE;
}

Predicate::Predicate(id_t classTag) :
        Predicate(classTag, PredicateType::STATE) {}

Predicate::Predicate(id_t classTag, PredicateType type) :
        ClassTag(classTag), type(type) {}

Predicate::Predicate(id_t classTag, PredicateType type, const Locus& loc) :
        ClassTag(classTag), type(type), location(loc) {}

PredicateType Predicate::getType() const {
   return type;
}

Predicate* Predicate::setType(PredicateType type) {
   this->type = type;
   return this;
}

const Locus& Predicate::getLocation() const {
   return location;
}

Predicate* Predicate::setLocations(const Locus& loc) {
   this->location = loc;
   return this;
}

unsigned Predicate::getNumOperands() const {
    return ops.size();
}

const Predicate::Operands& Predicate::getOperands() const {
    return ops;
}

std::string Predicate::toString() const {
   switch (type) {
   case PredicateType::REQUIRES: return "@R " + asString;
   case PredicateType::ENSURES:  return "@E " + asString;
   case PredicateType::ASSERT:   return "@A " + asString;
   case PredicateType::ASSUME:   return "@U " + asString;
   case PredicateType::PATH:     return "@P " + asString;
   case PredicateType::STATE:    return asString;
   default:                      return "@?" + asString;
   }
}

bool Predicate::equals(const Predicate* other) const {
   if (other == nullptr) return false;
   return classTag == other->classTag &&
           type == other->type &&
           util::equal(ops, other->ops, ops::deref_equals_to);
}

size_t Predicate::hashCode() const {
   return util::hash::defaultHasher()(classTag, type, ops);
}

Predicate* Predicate::clone() const {
    BYE_BYE(Predicate*, "Should not be called!");
}

bool operator==(const Predicate& a, const Predicate& b) {
   if (&a == &b) return true;
   return a.equals(&b);
}

std::ostream& operator<<(std::ostream& s, Predicate::Ptr p) {
    return s << p->toString();
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Predicate::Ptr p) {
    s << p->toString();
    if (with_predicate_locus(s)) {
        s << " at " << p->getLocation();
    }
    return s;
}

#include "Util/unmacros.h"

} // namespace borealis
