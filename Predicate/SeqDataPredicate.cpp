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
        const Locus& loc,
        PredicateType type) :
            Predicate(class_tag(*this), type, loc) {
    auto&& a = util::viewContainer(data)
                .map([](auto&& d) { return d->getName(); })
                .reduce("", [](auto&& acc, auto&& e) { return acc + "," + e; });

    asString = base->getName() + "=(" + a + ")";

    ops.insert(ops.end(), base);
    ops.insert(ops.end(), data.begin(), data.end());
}

Term::Ptr SeqDataPredicate::getBase() const {
    return ops[0];
}

auto SeqDataPredicate::getData() const -> decltype(util::viewContainer(ops)) {
    return util::viewContainer(ops).drop(1);
}

} /* namespace borealis */
