//
// Created by belyaev on 5/12/16.
//

#ifndef TERMSIZECALCULATOR_H
#define TERMSIZECALCULATOR_H

#include <unordered_set>

#include "State/PredicateState.def"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class TermSizeCalculator : public borealis::Transformer<TermSizeCalculator> {

    using Base = borealis::Transformer<TermSizeCalculator>;

public:

    TermSizeCalculator();

    Term::Ptr transformTerm(Term::Ptr term);
    Predicate::Ptr transformPredicate(Predicate::Ptr term);

    size_t getTermSize() const;
    size_t getPredicateSize() const;

    friend std::ostream& operator<<(std::ostream& ost, const TermSizeCalculator& tsc) {
        return ost << tsc.termSize << " terms; " << tsc.predicateSize << " predicates";
    }

    template<class T>
    static TermSizeCalculator measure(T&& what) {
        TermSizeCalculator ret;
        if(what) ret.transform(what);
        return ret;
    }

private:

    size_t termSize = 0;
    size_t predicateSize = 0;

};

} /* namespace borealis */

#endif //TERMSIZECALCULATOR_H
