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

std::ostream& operator<<(std::ostream& s, Predicate::Ptr p) {
    return s << p->toString();
}

} // namespace borealis
