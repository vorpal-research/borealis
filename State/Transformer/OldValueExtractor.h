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

// break ``@ensures( ... \old(x+y) ... )'' to
//       ``@assume(borealis.old.1 = x + y)'' && ``@ensures( ... borealis.old.1 ...)''
// where exactly this assumes should be put into depends on the current meaning of ``\old''
class OldValueExtractor: public borealis::Transformer<OldValueExtractor> {
    Locus locus;
    size_t seed = 0U;
    std::list<Annotation::Ptr> assumes;

    std::string getFreeName() {
        return "borealis.old." + util::toString(seed++);
    }

public:
    OldValueExtractor(
        const FactoryNest& FN,
        size_t seed = 0U
    ) : Transformer{ FN }, locus{}, seed{ seed }, assumes{} {}
    ~OldValueExtractor() {}

    Term::Ptr transformOpaqueCall(OpaqueCallTermPtr call) {
        auto builtin = llvm::dyn_cast<OpaqueBuiltinTerm>(call->getLhv());
        if( ! builtin || builtin->getVName() != "old" ) return call;
        if(call->getRhv().size() != 1) return call; // treated as an error in AnnotationMaterializer later
        auto arg = util::head(call->getRhv());

        auto genName = getFreeName();
        auto genTerm = FN.Term->getFreeVarTerm(arg->getType(), genName);

        auto predicate = FN.Term->getCmpTerm(llvm::ConditionType::EQ, genTerm, arg);
        std::vector<Term::Ptr> args { predicate };

        assumes.push_back(AssumeAnnotation::fromTerms(locus, "", args));
        return genTerm;
    }

    size_t getSeed() const { return seed; }
    const std::list<Annotation::Ptr>& getResults() const {
        return assumes;
    }
};

} /* namespace borealis */

#endif /* OLDVALUEEXTRACTOR_H_ */
