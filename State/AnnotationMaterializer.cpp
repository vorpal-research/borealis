/*
 * AnnotationMaterializer.cpp
 *
 *  Created on: Jan 21, 2013
 *      Author: belyaev
 */

#include "State/AnnotationMaterializer.h"

namespace borealis {

class AnnotationMaterializer::AnnotationMaterializerImpl {
public:
    const borealis::LogicAnnotation* A;
    borealis::TermFactory* TF;
    borealis::MetaInfoTrackerPass* MI;
    NameContext nc;
};

AnnotationMaterializer::AnnotationMaterializer(
        const borealis::LogicAnnotation& A,
        borealis::TermFactory* TF,
        borealis::MetaInfoTrackerPass* MI) :
                pimpl(
                        new AnnotationMaterializerImpl {
                            &A,
                            TF,
                            MI,
                            NameContext{ NameContext::Placement::GlobalScope, nullptr, A.getLocus() }
                        }
                ) {
    if (llvm::isa<EnsuresAnnotation>(A) ||
            llvm::isa<RequiresAnnotation>(A) ||
            llvm::isa<AssignsAnnotation>(A)) {
        pimpl->nc.placement = NameContext::Placement::OuterScope;
        if (auto f = llvm::dyn_cast_or_null<llvm::Function>(
                pimpl->MI->locate(pimpl->nc.loc, DiscoveryPolicy::NextFunction).val
            )) {
            pimpl->nc.func = f;
        }
    } else if (llvm::isa<AssertAnnotation>(A)) {
        pimpl->nc.placement = NameContext::Placement::InnerScope;
        if (auto f = llvm::dyn_cast_or_null<llvm::Function>(
                pimpl->MI->locate(pimpl->nc.loc, DiscoveryPolicy::PreviousFunction).val
            )) {
            pimpl->nc.func = f;
        }
    }
};

AnnotationMaterializer::~AnnotationMaterializer() {
    delete pimpl;
}

Annotation::Ptr AnnotationMaterializer::doit() {
    auto trm = transform(pimpl->A->getTerm());
    return pimpl->A->clone(trm);
}

MetaInfoTrackerPass::ValueDescriptor AnnotationMaterializer::forName(const std::string& name) const {
    switch(pimpl->nc.placement) {
    case NameContext::Placement::GlobalScope:
    case NameContext::Placement::InnerScope:
        return pimpl->MI->locate(name, pimpl->A->getLocus(), DiscoveryPolicy::PreviousVal);
    case NameContext::Placement::OuterScope:
        return pimpl->MI->locate(name, pimpl->A->getLocus(), DiscoveryPolicy::NextArgument);
    }
}

const NameContext& AnnotationMaterializer::nameContext() const {
    return pimpl->nc;
}

TermFactory& AnnotationMaterializer::factory() const {
    return *pimpl->TF;
}

void AnnotationMaterializer::failWith(const std::string& message) {
    static std::string buf;

    std::ostringstream str;
    str << "Error while processing annotation: "
        << *pimpl->A
        << "; scope "
        << pimpl->nc
        << ": "
        << message;

    buf = str.str();
    throw std::runtime_error(buf.c_str());
}

Annotation::Ptr materialize(
        Annotation::Ptr annotation,
        TermFactory* TF,
        MetaInfoTrackerPass* MI
            ) {
    if (auto* logic = llvm::dyn_cast<LogicAnnotation>(annotation)){
        AnnotationMaterializer am(*logic, TF, MI);
        return am.doit();
    } else {
        return annotation;
    }
}

} // namespace borealis
