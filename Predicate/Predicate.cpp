/*
 * Predicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/Predicate.h"

#include "Annotation/AssertAnnotation.h"
#include "Annotation/AssumeAnnotation.h"
#include "Annotation/EnsuresAnnotation.h"
#include "Annotation/RequiresAnnotation.h"

namespace borealis {

PredicateType predicateType(const Annotation* a) {
   using namespace llvm;

   if (isa<RequiresAnnotation>(a)) return PredicateType::REQUIRES;
   if (isa<EnsuresAnnotation>(a)) return PredicateType::ENSURES;
   if (isa<AssertAnnotation>(a)) return PredicateType::ASSERT;
   if (isa<AssumeAnnotation>(a)) return PredicateType::ASSUME;
   return PredicateType::STATE;
}

Predicate::Predicate(borealis::id_t predicate_type_id) :
        Predicate(predicate_type_id, PredicateType::STATE) {}

Predicate::Predicate(borealis::id_t predicate_type_id, PredicateType type) :
        predicate_type_id(predicate_type_id),
        type(type),
        location(nullptr) {}

Predicate::~Predicate() {}

std::ostream& operator<<(std::ostream& s, Predicate::Ptr p) {
    return s << p->toString();
}

} // namespace borealis
