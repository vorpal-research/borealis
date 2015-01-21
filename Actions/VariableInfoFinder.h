/*
 * VariableInfoFinder.h
 *
 *  Created on: Jan 21, 2015
 *      Author: belyaev
 */

#ifndef VARIABLEINFOFINDER_H_
#define VARIABLEINFOFINDER_H_

#include <clang/Frontend/FrontendAction.h>

#include "Codegen/VarInfo.h"

namespace borealis {

class VariableInfoFinder: public clang::ASTFrontendAction {
    struct VariableInfoFinderImpl;
    std::unique_ptr<VariableInfoFinderImpl> pimpl_;

public:
    virtual clang::ASTConsumer*
        CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override;

    VariableInfoFinder();
    virtual ~VariableInfoFinder();

    llvm::ArrayRef<VarInfo> vars() const;
};

} /* namespace borealis */

#endif /* VARIABLEINFOFINDER_H_ */
