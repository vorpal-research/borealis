/*
 * Predicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Annotation/AssertAnnotation.h"
#include "Annotation/AssumeAnnotation.h"
#include "Annotation/EnsuresAnnotation.h"
#include "Annotation/RequiresAnnotation.h"
#include "Predicate/Predicate.h"

namespace borealis {

PredicateType predicateType(const Annotation* a) {
   using namespace llvm;

   if (isa<RequiresAnnotation>(a)) return PredicateType::REQUIRES;
   if (isa<EnsuresAnnotation>(a)) return PredicateType::ENSURES;
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

} // namespace borealis
