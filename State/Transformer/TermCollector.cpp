/*
 * TermCollector.cpp
 *
 *  Created on: Mar 18, 2015
 *      Author: ice-phoenix
 */

#include "State/Transformer/TermCollector.h"

#include "Util/util.h"

namespace borealis {

TermCollector::TermCollector(FactoryNest FN) : Base(FN) {}

const std::unordered_set<Term::Ptr, TermHash, TermEquals>& TermCollector::getTerms() const {
    return terms;
}

Term::Ptr TermCollector::transformTerm(Term::Ptr term) {
    terms.insert(term);
    return Base::transformTerm(term);
}

} /* namespace borealis */
