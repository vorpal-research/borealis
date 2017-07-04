#ifndef STATE_TRANSFORMER_TERMCOLLECTOR_H_
#define STATE_TRANSFORMER_TERMCOLLECTOR_H_

#include <unordered_set>

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

namespace borealis {
struct DefaultTermCollectorFilter {
    template<class T> bool operator()(T&&) const { return true; }
};

template<class Filter = DefaultTermCollectorFilter>
class TermCollector : public borealis::Transformer<TermCollector<Filter>> {

    using Base = borealis::Transformer<TermCollector<Filter>>;

public:

    TermCollector(FactoryNest FN): Base(FN), filter{} {}
    TermCollector(FactoryNest FN, Filter f): Base(FN), filter(f) {}

    Term::Ptr transformTerm(Term::Ptr term) {
        if(filter(term)) terms.insert(term);
        return Base::transformTerm(term);
    }

    const std::unordered_set<Term::Ptr, TermHash, TermEquals>& getTerms() const {
        return terms;
    }
    std::unordered_set<Term::Ptr, TermHash, TermEquals> moveTerms() {
        return std::move(terms);
    }

private:

    std::unordered_set<Term::Ptr, TermHash, TermEquals> terms;
    Filter filter; // XXX: empty base optimization or no matter?

};

} /* namespace borealis */

#endif /* STATE_TRANSFORMER_TERMCOLLECTOR_H_ */
