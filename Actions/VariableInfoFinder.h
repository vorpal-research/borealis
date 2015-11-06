/*
 * VariableInfoFinder.h
 *
 *  Created on: Jan 21, 2015
 *      Author: belyaev
 */

#ifndef VARIABLEINFOFINDER_H_
#define VARIABLEINFOFINDER_H_

#include <clang/Frontend/FrontendAction.h>

#include "Codegen/CType/CTypeFactory.h"
#include "Codegen/VarInfo.h"

namespace borealis {

struct ExtVariableInfoData {
    std::unordered_set<VarInfo> vars;
    CTypeContext::Ptr types;

    auto begin() const { return vars.begin(); }
    auto end() const { return vars.end(); }
};

class VariableInfoFinder: public clang::ASTFrontendAction {
    struct VariableInfoFinderImpl;
    std::unique_ptr<VariableInfoFinderImpl> pimpl_;

public:
    virtual clang::ASTConsumer*
        CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override;

    VariableInfoFinder(CTypeFactory* ctf);
    virtual ~VariableInfoFinder();

    ExtVariableInfoData vars() const;
};

} /* namespace borealis */

#endif /* VARIABLEINFOFINDER_H_ */
