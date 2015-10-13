//
// Created by belyaev on 4/22/15.
//

#ifndef EXTERNAL_FUNCTION_MATERIALIZER_H
#define EXTERNAL_FUNCTION_MATERIALIZER_H

#include "Annotation/AssertAnnotation.h"
#include "State/Transformer/Transformer.hpp"

namespace borealis {

class ExternalFunctionMaterializer: public Transformer<ExternalFunctionMaterializer> {

    llvm::CallSite theCallSite;
    VariableInfoTracker* VIT;
    Locus locus;
public:
    ExternalFunctionMaterializer(
            FactoryNest const& FN, llvm::CallSite theCallSite, VariableInfoTracker* VIT, const Locus& locus
        )
        : Transformer(FN), theCallSite(theCallSite), VIT(VIT), locus(locus) { }

    void failWith(llvm::StringRef message) const {
        throw std::runtime_error(message.str());
    }

    void failWith(std::string&& message) const {
        throw std::runtime_error(std::move(message));
    }

    Term::Ptr transformOpaqueBuiltinTerm(OpaqueBuiltinTermPtr term) {
        if(llvm::StringRef(term->getVName()).startswith("arg")) {
            unsigned ix = 0;
            if(term->getVName().size() > 3) {
                if(llvm::StringRef(term->getVName()).drop_front(3).getAsInteger(10, ix)) {
                    failWith("Illegal argument term: " + term->getName());
                }
            }
            if(ix >= theCallSite.arg_size()) failWith("Illegal argument term: " + term->getName());

            auto arg = theCallSite.getArgument(ix);
            for(auto&& argInfo : util::view(VIT->getVars().get(arg))) {
                borealis::DIType dt = argInfo.second.type;
                auto argSignedness = dt.getSignedness();
                return FN.Term->getValueTerm(arg, argSignedness);
            }

            return FN.Term->getValueTerm(arg);

        } else if(term->getVName() == "result") {
            auto res = theCallSite.getInstruction();
            for (auto&& resInfo : util::view(VIT->getVars().get(res))) {
                borealis::DIType dt = resInfo.second.type;
                auto resSignedness = dt.getSignedness();
                return FN.Term->getValueTerm(res, resSignedness);
            }

            return FN.Term->getValueTerm(res);
        } else if(term->getVName() == "num_args") {
            auto res = theCallSite.arg_size();
            return FN.Term->getIntTerm(res, FN.Type->getInteger());
        } else return term;
    }

    Annotation::Ptr transformRequiresAnnotation(RequiresAnnotationPtr anno) {
        return std::make_shared<AssertAnnotation>(locus, anno->getMeta(), anno->getTerm());
    }

    Annotation::Ptr transformEnsuresAnnotation(EnsuresAnnotationPtr anno) {
        return std::make_shared<AssumeAnnotation>(locus, anno->getMeta(), anno->getTerm());
    }
};

} /* namespace borealis */

#endif /* EXTERNAL_FUNCTION_MATERIALIZER_H */
