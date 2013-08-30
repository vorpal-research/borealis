/*
 * clang_pipeline.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/Utils.h>

#include "clang/Frontend/Utils.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/Utils.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Arg.h"
#include "clang/Driver/ArgList.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/Tool.h"
#include "llvm/Support/Host.h"

#include <iostream>

#include "Driver/clang_pipeline.h"
#include "Util/util.hpp" // ref<T>

using namespace clang;

static CompilerInvocation *
createInvocationFromCommandLine(ArrayRef<const char *> ArgList,
                            IntrusiveRefCntPtr<DiagnosticsEngine> Diags) {

  if (!Diags.getPtr()) {
    // No diagnostics engine was provided, so create our own diagnostics object
    // with the default options.
    DiagnosticOptions DiagOpts;
    Diags = CompilerInstance::createDiagnostics(DiagOpts, ArgList.size(),
                                                ArgList.begin());
  }

  SmallVector<const char *, 16> Args;
  Args.push_back("<clang>"); // FIXME: Remove dummy argument.
  Args.insert(Args.end(), ArgList.begin(), ArgList.end());

  // FIXME: We shouldn't have to pass in the path info.
  driver::Driver TheDriver("clang", llvm::sys::getDefaultTargetTriple(),
                           "a.out", false, *Diags);

  // Don't check that inputs exist, they may have been remapped.
  TheDriver.setCheckInputsExist(false);

  OwningPtr<driver::Compilation> C(TheDriver.BuildCompilation(Args));

  // FIXME: remove the comments after the compile-link stuff will have been sorted out
//  std::cerr << "compile " << " ";
//  for(const auto& arg: C->getArgsForToolChain(&C->getDefaultToolChain(), nullptr)) {
//      std::cerr << arg->getAsString(C->getArgs()) << " ";
//  }
//  std::cerr << std::endl;

  const driver::JobList &Jobs = C->getJobs();

//  for(const auto& job: Jobs) {
//      const driver::Command *Cmd = cast<driver::Command>(job);
//      const driver::ArgStringList &CCArgs = Cmd->getArguments();
//
//      std::cerr << Cmd->getCreator().getName() << " ";
//      for(const auto& arg: CCArgs) {
//          std::cerr << arg << " ";
//      }
//      std::cerr << std::endl;
//  }

  const driver::Command *Cmd = cast<driver::Command>(*Jobs.begin());
  if (StringRef(Cmd->getCreator().getName()) != "clang") {
    Diags->Report(diag::err_fe_expected_clang_command);
    return 0;
  }

  const driver::ArgStringList &CCArgs = Cmd->getArguments();
  OwningPtr<CompilerInvocation> CI(new CompilerInvocation());
  if (!CompilerInvocation::CreateFromArgs(*CI,
                                      CCArgs.data(),
                                      CCArgs.data() +
                                      CCArgs.size(),
                                     *Diags))
    return 0;
  return CI.take();
}


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
    pimpl->ci.setInvocation(::createInvocationFromCommandLine(args, diags));

    if(pimpl->ci.hasInvocation()) {
        pimpl->ci.getCodeGenOpts().EmitDeclMetadata = true;
        pimpl->ci.getCodeGenOpts().DebugInfo = true;

        pimpl->ci.getDiagnosticOpts().ShowCarets = false;
    } else {
        debug_break();
    }
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
