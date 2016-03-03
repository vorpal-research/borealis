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
    CTypeFactory* CTF;

public:
    VariableInfoFinderVisitor(CTypeFactory* CTF): CTF(CTF) {}

    void setASTContext(const clang::ASTContext* ac) {
        ac_ = ac;
        sm_ = &ac->getSourceManager();
    }

    std::unordered_set<clang::Decl*> visited;
    llvm::StringRef getName(clang::NamedDecl* d) {
        auto id = d->getIdentifier();
        if(!id) return "";
        else return id->getName();
    }

    bool VisitDeclRefExpr(clang::DeclRefExpr* del) {
        auto decl = del->getDecl();
        if(visited.count(decl)) return true;
        visited.insert(decl);

        if(auto&& fun = dyn_cast<clang::FunctionDecl>(decl)) {
            handleFunctionDecl(fun);
        }
        else handleDecl(decl, getName(decl));
        return true;
    }

    void handleFunctionDecl(clang::FunctionDecl* decl) {
        // this magically handles renamed functions
        // like __isoc99_scanf instead of scanf
        if(auto&& asm_ = decl->getAttr<clang::AsmLabelAttr>()) {
            handleDecl(decl, asm_->getLabel());
        }
        CTF->getType(decl->getType(), *ac_);
        handleDecl(decl, getName(decl));

        llvm::StringRef prefix = "__builtin_";
        if(getName(decl).startswith(prefix)) {
            handleDecl(decl, getName(decl).drop_front(prefix.size()));
        }

        for (auto&& param : decl->parameters()) {
            handleDecl(param, getName(param));
        }
    }

    void handleDecl(clang::ValueDecl* decl, llvm::StringRef name) {
        if(name == "") return;

        VarInfo info;
        ON_SCOPE_EXIT(vars_.push_back(std::move(info)));
        info.type = CTF->getRef(CTF->getType(decl->getType(), *ac_));
        info.name = name;
        info.locus = Locus{decl->getLocation(), *sm_};
        info.storage = StorageSpec::Unknown;
        if(!decl->isDefinedOutsideFunctionOrMethod()) info.kind = VariableKind::Local;
        else switch (decl->getLinkageAndVisibility().getLinkage()) {
            case clang::Linkage::ExternalLinkage:
            case clang::Linkage::UniqueExternalLinkage:
                info.kind = VariableKind::Extern;
            default:
                info.kind = VariableKind::Global;
        }
    }

    bool VisitQualifiedTypeLoc(clang::QualifiedTypeLoc typeloc) {
        CTF->getType(typeloc.getType(), *ac_);
        return true;
    }

    //bool VisitFunctionDecl(clang::FunctionDecl* decl) {
    //    CTF->getType(decl->getType(), *ac_);
    //    handleDecl(decl);
    //    for (auto&& param : decl->parameters()) {
    //        handleDecl(param);
    //    }
    //    return true;
    //}
//
    //bool VisitVarDecl(clang::VarDecl* decl) {
    //    handleDecl(decl);
    //    return true;
    //}

    llvm::ArrayRef<VarInfo> vars() const {
        return vars_;
    }

    CTypeContext::Ptr getTypeContext() const {
        return CTF->getCtx();
    }
};

class VariableInfoFinderConsumer : public clang::ASTConsumer {
public:
    VariableInfoFinderConsumer(VariableInfoFinderVisitor& visitor) : visitor_(&visitor) {};

    virtual void HandleTranslationUnit(clang::ASTContext& Context) {
        // Traversing the translation unit decl via a RecursiveASTVisitor
        // will visit all nodes in the AST.
        visitor_->setASTContext(&Context);
        visitor_->TraverseDecl(Context.getTranslationUnitDecl());
    }

    llvm::ArrayRef<VarInfo> vars() const {
        return visitor_->vars();
    }

    CTypeContext::Ptr getTypeContext() const {
        return visitor_->getTypeContext();
    }

private:
    // A RecursiveASTVisitor implementation.
    // @notnull
    VariableInfoFinderVisitor* visitor_;
};

}
/* namespace */

struct VariableInfoFinder::VariableInfoFinderImpl {
    VariableInfoFinderVisitor visitor;

    VariableInfoFinderImpl(CTypeFactory* ctf) : visitor(ctf) { }

};

clang::ASTConsumer* VariableInfoFinder::CreateASTConsumer(
    clang::CompilerInstance&,
    llvm::StringRef
) {
    return new VariableInfoFinderConsumer(pimpl_->visitor);
}


VariableInfoFinder::VariableInfoFinder(CTypeFactory* CTF) :
    pimpl_{
        util::make_unique<VariableInfoFinderImpl>(CTF)
    } { }

VariableInfoFinder::~VariableInfoFinder() { }

ExtVariableInfoData VariableInfoFinder::vars() const {
    return ExtVariableInfoData{ util::viewContainer(pimpl_->visitor.vars()).toHashSet(), pimpl_->visitor.getTypeContext() };
}

} /* namespace borealis */


#include "Util/unmacros.h"
