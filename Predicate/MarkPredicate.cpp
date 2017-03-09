#include "Predicate/MarkPredicate.h"

#include <tinyformat/tinyformat.h>

namespace borealis {

MarkPredicate::MarkPredicate(
    Term::Ptr id,
    const Locus& loc,
    PredicateType type) :
    Predicate(class_tag(*this), type, loc) {
    asString = tfm::format("mark(%s)", id->getName());
    ops = { id };
}

Term::Ptr MarkPredicate::getId() const {
    return ops.front();
}

} /* namespace borealis */
