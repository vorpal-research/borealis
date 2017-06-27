//
// Created by belyaev on 4/21/15.
//

#ifndef ANNOTATION_SUBSTITUTOR_H
#define ANNOTATION_SUBSTITUTOR_H

#include "State/Transformer/Transformer.hpp"
#include "State/Transformer/TermCollector.h"

#include "Util/macros.h"

namespace borealis {

static std::set<Term::Ptr, TermCompare> substitutionOrdering(Annotation::Ptr anno) {
    TermCollector<> CT{FactoryNest{}};
    CT.transform(anno);
    return util::viewContainer(CT.getTerms())
          .filter([&](Term::Ptr t){ return llvm::is_one_of<ValueTerm, ArgumentTerm, ReturnValueTerm>(t); })
          .toSet<TermCompare>();
}

class AnnotationSubstitutor: public Transformer<AnnotationSubstitutor> {
    std::vector<llvm::Value*> vars;
    std::unordered_map<Term::Ptr, llvm::Value*, TermHash, TermEquals> mapping;

    using Base = Transformer<AnnotationSubstitutor>;

public:
    AnnotationSubstitutor(const FactoryNest& FN, std::vector<llvm::Value*> vars)
        : Transformer(FN), vars(vars) { }

    using Base::transform;

    Annotation::Ptr transformLogic(LogicAnnotationPtr anno) {
        auto ordering = substitutionOrdering(anno);
        ASSERTC(ordering.size() == vars.size());
        mapping = util::viewContainer(ordering).zipWith(util::viewContainer(vars)).to<decltype(mapping)>();
        return Base::transformLogic(anno);
    }

    Term::Ptr transformTerm(Term::Ptr term) {
        if(mapping.count(term)) {
            auto value = mapping.at(term);
            llvm::Signedness signedness = llvm::Signedness::Unknown;
            if(auto inttype = llvm::dyn_cast<type::Integer>(term->getType())) {
                signedness = inttype->getSignedness();
            }
            return FN.Term->getValueTerm(value, signedness);
        } // the type is expected to be fixed by materializer later
        else return Base::transformTerm(term);
    }

};

static Annotation::Ptr substituteAnnotationCall(const FactoryNest& FN, llvm::CallSite ci) {
    AnnotationSubstitutor AS(
        FN,
        util::view(ci.arg_begin(), ci.arg_end())
            .drop(1)
            .map(LAM(it, static_cast<llvm::Value*>(it)))
            .toVector()
    );
    return AS.transform(Annotation::fromIntrinsic(ci));
}

} /* namespace borealis */

#include "Util/unmacros.h"

#endif /* ANNOTATION_SUBSTITUTOR_H */
