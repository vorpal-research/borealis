//
// Created by ice-phoenix on 5/20/15.
//

#ifndef SANDBOX_REANIMATOR_H
#define SANDBOX_REANIMATOR_H

#include "SMT/Result.h"
#include "State/Transformer/ArrayBoundsCollector.h"
#include "Term/Term.h"

namespace borealis {

class Reanimator {

public:

    using ArrayBounds = std::unordered_set<long long>;
    using ArrayBoundsMap = std::unordered_map<unsigned long long, ArrayBounds>;

    Reanimator(const smt::SatResult& result, const ArrayBoundsCollector::ArrayBounds& arrayBounds);

    const smt::SatResult& getResult() const;

    const ArrayBoundsMap& getArrayBoundsMap() const;

    const ArrayBounds& getArrayBounds(unsigned long long base) const;

    long long getArraySize(unsigned long long base) const;

private:

    smt::SatResult result;
    ArrayBoundsMap arrayBoundsMap;

    void processArrayBounds(const ArrayBoundsCollector::ArrayBounds& arrayBounds);

};

class ReanimatorView {
public:
    ReanimatorView(const Reanimator& r, Term::Ptr term);

    friend borealis::logging::logstream& operator<<(
        borealis::logging::logstream& s,
        const ReanimatorView& rv
    );

private:
    Reanimator r;
    Term::Ptr term;
};

ReanimatorView raise(const Reanimator& r, Term::Ptr value);

} // namespace borealis

#endif //SANDBOX_REANIMATOR_H
