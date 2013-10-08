/*
 * SeqDataPredicate.cpp
 *
 *  Created on: Sep 25, 2012
 *      Author: ice-phoenix
 */

#include "Predicate/SeqDataPredicate.h"

namespace borealis {

SeqDataPredicate::SeqDataPredicate(
        Term::Ptr base,
        const std::vector<Term::Ptr>& data,
        PredicateType type) :
            Predicate(class_tag(*this), type),
            base(base),
            data(data) {

    std::string a{""};

    if (!data.empty()) {
        a = util::head(data)->getName();
        for (const auto& g : util::tail(data)) {
            a = a + "," + g->getName();
        }
    }

    asString = base->getName() + "=(" + a + ")";
}

bool SeqDataPredicate::equals(const Predicate* other) const {
    if (const Self* o = llvm::dyn_cast_or_null<Self>(other)) {
        return Predicate::equals(other) &&
                *base == *o->base &&
                util::equal(data, o->data,
                    [](const Term::Ptr& a, const Term::Ptr& b) { return *a == *b; }
                );
    } else return false;
}

size_t SeqDataPredicate::hashCode() const {
    return util::hash::defaultHasher()(Predicate::hashCode(), base, data);
}

} /* namespace borealis */
