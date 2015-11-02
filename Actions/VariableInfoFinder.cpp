/*
 * VariableInfoFinder.cpp
 *
 *  Created on: Jan 21, 2015
 *      Author: belyaev
 */

#include <memory>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DataRecursiveASTVisitor.h>


#include "Actions/VariableInfoFinder.h"
#include "Codegen/CType/CTypeFactory.h"
#include "Util/util.hpp"

#include "Util/macros.h"

namespace borealis {

namespace {

class VariableInfoFinderVisitor
    : public clang::DataRecursiveASTVisitor<VariableInfoFinderVisitor> {
    std::vector<VarInfo> vars_;
    const clang::SourceManager* sm_ = nullptr;
    const clang::ASTContext* ac_ = nullptr;
    CTypeFactory CTF;

public:
    void setASTContext(const clang::ASTContext* ac) {
        ac_ = ac;
        sm_ = &ac->getSourceManager();
    }

    void handleDecl(clang::ValueDecl* decl) {
        VarInfo info;
        ON_SCOPE_EXIT(vars_.push_back(std::move(info)));
        info.type = CTF.getType(decl->getType(), *ac_);
        info.name = decl->getName();
        info.locus = Locus{decl->getLocation(), *sm_};
        info.storage = StorageSpec::Unknown,
            info.kind = (!decl->isDefinedOutsideFunctionOrMethod()) ? VariableKind::Local
                                                                    : (decl->getLinkageAndVisibility().getLinkage() ==
                                                                       clang::Linkage::ExternalLinkage
                                                                       ||
                                                                       decl->getLinkageAndVisibility().getLinkage() ==
                                                                       clang::Linkage::UniqueExternalLinkage)
                                                                      ? VariableKind::Extern
                                                                      : VariableKind::Global;
    }

    bool VisitQualifiedTypeLoc(clang::QualifiedTypeLoc typeloc) {
        CTF.getType(typeloc.getType(), *ac_);
        return true;
    }

    bool VisitFunctionDecl(clang::FunctionDecl* decl) {
        CTF.getType(decl->getType(), *ac_);
        handleDecl(decl);
        for (auto&& param : decl->parameters()) {
            handleDecl(param);
        }
        return true;
    }

    bool VisitVarDecl(clang::VarDecl* decl) {
        handleDecl(decl);
        return true;
    }

    llvm::ArrayRef<VarInfo> vars() const {
        return vars_;
    }
};

class VariableInfoFinderConsumer : public clang::ASTConsumer {
public:
    virtual void HandleTranslationUnit(clang::ASTContext& Context) {
        // Traversing the translation unit decl via a RecursiveASTVisitor
        // will visit all nodes in the AST.
        Visitor.setASTContext(&Context);
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

    llvm::ArrayRef<VarInfo> vars() const {
        return Visitor.vars();
    }

private:
    // A RecursiveASTVisitor implementation.
    VariableInfoFinderVisitor Visitor;
};

}
/* namespace */

struct VariableInfoFinder::VariableInfoFinderImpl {
    std::unique_ptr<VariableInfoFinderConsumer> consumer;

    VariableInfoFinderImpl(decltype(consumer)&& consumer) : consumer(std::move(consumer)) { }

};

clang::ASTConsumer* VariableInfoFinder::CreateASTConsumer(
    clang::CompilerInstance&,
    llvm::StringRef
) {
    return pimpl_->consumer.get();
}


VariableInfoFinder::VariableInfoFinder() :
    pimpl_{
        util::make_unique<VariableInfoFinderImpl>(
            util::make_unique<VariableInfoFinderConsumer>()
        )
    } { }

VariableInfoFinder::~VariableInfoFinder() { }

llvm::ArrayRef<VarInfo> VariableInfoFinder::vars() const {
    return pimpl_->consumer->vars();
}

} /* namespace borealis */


#include "Util/unmacros.h"
