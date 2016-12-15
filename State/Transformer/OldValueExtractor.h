/*
 * OldValueExtractor.h
 *
 *  Created on: Feb 7, 2014
 *      Author: belyaev
 */

#ifndef OLDVALUEEXTRACTOR_H_
#define OLDVALUEEXTRACTOR_H_

#include <unordered_map>
#include <string>

#include "Annotation/Annotation.def"

#include "Transformer.hpp"

namespace borealis {

// \old(X) -> X
class OldValueEraser: public borealis::Transformer<OldValueEraser> {
public:
    OldValueEraser(const FactoryNest &FN) : Transformer(FN) {}

    Term::Ptr transformOpaqueCallTerm(OpaqueCallTermPtr call);
};

// break ``@ensures( ... \old(x+y) ... )'' to
//       ``@assume(borealis.old.1 = x + y)'' && ``@ensures( ... borealis.old.1 ...)''
// where exactly this assumes should be put into depends on the current meaning of ``\old''
class OldValueExtractor: public borealis::Transformer<OldValueExtractor> {
    Locus locus;
    size_t seed = 0U;
    std::list<Annotation::Ptr> assumes;
    OldValueEraser eraser;

    std::string getFreeName() {
        return "borealis.old." + util::toString(seed++);
    }

public:
    OldValueExtractor(
        const FactoryNest& FN,
        size_t seed = 0U
    ) : Transformer{ FN }, locus{}, seed{ seed }, assumes{}, eraser{FN} {}
    ~OldValueExtractor() {}

    Term::Ptr transformOpaqueCallTerm(OpaqueCallTermPtr call);

    size_t getSeed() const { return seed; }
    const std::list<Annotation::Ptr>& getResults() const {
        return assumes;
    }
};

} /* namespace borealis */

#endif /* OLDVALUEEXTRACTOR_H_ */
