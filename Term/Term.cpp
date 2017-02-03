/*
 * Term.cpp
 *
 *  Created on: Nov 16, 2012
 *      Author: ice-phoenix
 */

#include <z3/z3++.h>
#include "Statistics/statistics.h"
#include "Term/Term.h"
#include "Term/TermFactory.h"

namespace borealis {

static Statistic totalTermsCreated("misc", "totalTerms", "Total number of terms created");

Term::Term(id_t classTag, Type::Ptr type, const std::string& name):
    ClassTag(classTag), type(type), name(name) { ++totalTermsCreated; };

Term::Term(id_t classTag, Type::Ptr type, util::indexed_string name):
    ClassTag(classTag), type(type), name(name) { ++totalTermsCreated; };

Term::Term(id_t classTag, Type::Ptr type, const char* name):
    Term(classTag, type, util::indexed_string(name)) {};

void Term::update() {}

Term::Ptr Term::setType(TermFactory* TF, Type::Ptr newtype) const {
    return TF->setType(newtype, shared_from_this());
}

Type::Ptr Term::getType() const {
    return type;
}

const std::string& Term::getName() const {
    return name.str();
}

size_t Term::getNumSubterms() const {
    return subterms.size();
}

const Term::Subterms& Term::getSubterms() const {
    return subterms;
}

Term::Set Term::getFullTermSet(Term::Ptr term) {
    auto&& res = Term::Set{term};
    for (auto&& subterm : term->subterms) {
        auto&& nested = Term::getFullTermSet(subterm);
        res.insert(nested.begin(), nested.end());
    }
    return res;
}

bool Term::equals(const Term* other) const {
    if (other == nullptr) return false;
    return classTag == other->classTag &&
            // type == other->type &&
            name == other->name &&
            // retypable == other->retypable &&
            util::equal(subterms, other->subterms, ops::deref_equals_to);
}

size_t Term::hashCode() const {
    return util::hash::defaultHasher()(classTag, /*type,*/ name, /*retypable,*/ subterms);
}

bool operator==(const Term& a, const Term& b) {
    if (&a == &b) return true;
    else return a.equals(&b);
}

std::ostream& operator<<(std::ostream& s, Term::Ptr t) {
    return s << t->getName();
}

borealis::logging::logstream& operator<<(borealis::logging::logstream& s, Term::Ptr t) {
    return s << t->getName();
}

} // namespace borealis
