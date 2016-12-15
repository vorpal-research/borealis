/*
 * OldValueExtractor.cpp
 *
 *  Created on: Feb 7, 2014
 *      Author: belyaev
 */

#include "State/Transformer/OldValueExtractor.h"

namespace borealis {

using namespace functional_hell::matchers;
using namespace functional_hell::matchers::placeholders;

// local pattern matching \old(%pat%)
template<class Pat>
static auto $Old(Pat pat){
    return $OpaqueCallTerm($OpaqueBuiltinTerm("old"), BSeq(pat));
};

Term::Ptr OldValueEraser::transformOpaqueCallTerm(Transformer::OpaqueCallTermPtr call) {
    if(auto m = $Old(_1) >> call) {
        return m->_1;
    } else return call;
}


Term::Ptr OldValueExtractor::transformOpaqueCallTerm(Transformer::OpaqueCallTermPtr call) {
    if(auto m = $Old(_1) >> call) {
        Term::Ptr arg = m->_1;
        auto genName = getFreeName();
        auto genTerm = FN.Term->getFreeVarTerm(arg->getType(), genName);

        auto predicate = FN.Term->getCmpTerm(llvm::ConditionType::EQ, genTerm, arg);
        std::vector<Term::Ptr> args { predicate };

        assumes.push_back(eraser.transform(AssumeAnnotation::fromTerms(locus, "", args)));
        return genTerm;
    } else return call; // old with more arguments must be rejected in the materializer phase
}

} /* namespace borealis */
