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
#include "Util/util.hpp"

#include "Util/macros.h"

namespace borealis {

namespace {

class VariableInfoFinderVisitor
    : public clang::DataRecursiveASTVisitor<VariableInfoFinderVisitor> {
    std::vector<VarInfo> vars_;
    const clang::SourceManager* sm_ = nullptr;

    public:
    void setSourceManager(const clang::SourceManager* sm) {
        sm_ = sm;
    }

    bool VisitFunctionDecl(clang::FunctionDecl* decl) {
        // For debugging, dumping the AST nodes will show which nodes are already
        // being visited.
        {
            VarInfo funcInfo;
            ON_SCOPE_EXIT(vars_.push_back(std::move(funcInfo)));

            funcInfo.originalName = util::just(decl->getName().str());
            funcInfo.originalLocus = util::just(Locus{ decl->getLocation(), *sm_ });
            funcInfo.treatment = VarInfo::Plain;
            funcInfo.ast = decl;
        }
        for(auto&& param : decl->parameters()) {
            VarInfo paramInfo;
            ON_SCOPE_EXIT(vars_.push_back(std::move(paramInfo)));

            paramInfo.originalName = util::just(decl->getName().str());
            paramInfo.originalLocus = util::just(Locus{ param->getLocation(), *sm_ });
            paramInfo.treatment = VarInfo::Plain;
            paramInfo.ast = decl;
        }
        // The return value indicates whether we want the visitation to proceed.
        // Return false to stop the traversal of the AST.
        return true;
    }

    llvm::ArrayRef<VarInfo> vars() const {
        return vars_;
    }
};

class VariableInfoFinderConsumer : public clang::ASTConsumer {
public:
    virtual void HandleTranslationUnit(clang::ASTContext &Context) {
        // Traversing the translation unit decl via a RecursiveASTVisitor
        // will visit all nodes in the AST.
        Visitor.setSourceManager(&Context.getSourceManager());
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

    llvm::ArrayRef<VarInfo> vars() const {
        return Visitor.vars();
    }
private:
    // A RecursiveASTVisitor implementation.
    VariableInfoFinderVisitor Visitor;
};

} /* namespace */

struct VariableInfoFinder::VariableInfoFinderImpl {
    std::unique_ptr<VariableInfoFinderConsumer> consumer;

    VariableInfoFinderImpl(decltype(consumer)&& consumer): consumer(std::move(consumer)) {}

};

clang::ASTConsumer* VariableInfoFinder::CreateASTConsumer(
        clang::CompilerInstance&,
        llvm::StringRef
    ) {
    return pimpl_->consumer.get();
}



VariableInfoFinder::VariableInfoFinder():
    pimpl_{
        util::make_unique<VariableInfoFinderImpl>(
            util::make_unique<VariableInfoFinderConsumer>()
        )
    }{}

VariableInfoFinder::~VariableInfoFinder(){}

llvm::ArrayRef<VarInfo> VariableInfoFinder::vars() const {
    return pimpl_->consumer->vars();
}

} /* namespace borealis */


#include "Util/unmacros.h"
