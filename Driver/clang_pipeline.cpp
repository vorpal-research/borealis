/*
 * clang_pipeline.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/Utils.h>
#include <iostream>

#include "Driver/clang_pipeline.h"
#include "Util/util.hpp" // ref<T>

namespace borealis {
namespace driver {

struct clang_pipeline::impl {
    clang::CompilerInstance ci;
    std::vector<util::ref<clang::FrontendAction>> actions;
};

clang_pipeline::~clang_pipeline() {};

clang_pipeline::clang_pipeline(
        const std::string&,
        const std::vector<const char*>& args,
        const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags
    ): pimpl{ new impl{} } {
    pimpl->ci.setDiagnostics(diags.getPtr());
    pimpl->ci.setInvocation(clang::createInvocationFromCommandLine(args, diags));
    pimpl->ci.getCodeGenOpts().EmitDeclMetadata = true;
    pimpl->ci.getCodeGenOpts().DebugInfo = true;
}

std::vector<std::string> clang_pipeline::getInvocationArgs() const {
    // this is generally fucked up
    // we deliberately remove constness from ci 'cos
    // ci.getInvocation() is not const
    auto& ci = const_cast<clang::CompilerInstance&>(pimpl->ci);
    // hereby we swear to Bible that we won't change anything in ci

    if(!ci.hasInvocation()) return std::vector<std::string>{};

    std::vector<std::string> ret;
    // both getInvocation() and toArgs() are not declared const for some dirty reason
    ci.getInvocation().toArgs(ret);
    return std::move(ret);
}

clang::SourceManager& clang_pipeline::getSourceManager() const {
    return pimpl->ci.getSourceManager();
}

void clang_pipeline::add(clang::FrontendAction& action) {
    pimpl->actions.push_back(util::ref<clang::FrontendAction>(action));
}

clang_pipeline::status clang_pipeline::run() {
    if(!pimpl->ci.hasInvocation()) return status::FAILURE;

    bool success = true;

    for(auto& action: pimpl->actions) {
        success &= pimpl->ci.ExecuteAction(action);
    }

    return success ? status::SUCCESS : status::FAILURE;
}

} // namespace driver
} // namespace borealis
