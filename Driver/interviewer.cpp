/*
 * interviewer.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <clang/Basic/Diagnostic.h>
#include <clang/Driver/Action.h>
#include <clang/Driver/Arg.h>
#include <clang/Driver/ArgList.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Option.h>
#include <clang/Driver/Options.h>
#include <clang/Driver/OptTable.h>

#include <llvm/Support/Program.h>
#include <llvm/Support/Host.h> // getDefaultTargetTriple

#include "Config/config.h"
#include "Driver/interviewer.h"
#include "Logging/logger.hpp"
#include "Util/util.h"

#include "Util/macros.h"

namespace borealis {
namespace driver {

struct interviewer::impl {
    llvm::sys::Path pathToExecutable;
    llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags;
    std::unique_ptr<clang::driver::Driver> theDriver;
    std::unique_ptr<clang::driver::Compilation> theCompilation;

    impl(
        const std::string& what,
        const std::vector<const char*>&,
        const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags,
        const borealis::config::StringConfigEntry& config
    ) : pathToExecutable{
            config.get<llvm::sys::Path>()
                  .getOrElse(llvm::sys::Program::FindProgramByName(what))
        },
        diags{ diags },
        theDriver{},
        theCompilation{} {};
};

interviewer::~interviewer() {};

interviewer::interviewer(
    const std::string& what,
    const std::vector<const char*>& args,
    const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags,
    const borealis::config::StringConfigEntry& config
) : pimpl{ new impl{ what, args, diags, config } } {

    auto invoke_args = std::vector<const char*>{};
    invoke_args.reserve(args.size() + 1);
    invoke_args.push_back(pimpl->pathToExecutable.c_str());
    invoke_args.insert(invoke_args.end(), args.begin(), args.end());

    pimpl->theDriver = borealis::util::uniq(new clang::driver::Driver{
        invoke_args.front(), llvm::sys::getDefaultTargetTriple(), "a.out", true, *diags
    });
    pimpl->theCompilation = borealis::util::uniq(pimpl->theDriver->BuildCompilation(invoke_args));

};

std::vector<command> interviewer::getCompileCommands() {
    using borealis::util::streams::error;

    std::vector<command> ret;
    const auto& optTable = pimpl->theDriver->getOpts();

    if (!pimpl->theCompilation) {
        errs() << error("Compilation object not exists") << endl;
        return std::move(ret);
    }

    auto& jobs = pimpl->theCompilation->getJobs();
    for (auto* job : jobs) {
        if (auto* command = clang::dyn_cast<clang::driver::Command>(job)) {
            borealis::driver::command toPut;
            const auto& args = command->getArguments();

            unsigned missingArgIndex, missingArgCount;

            toPut.cl = std::shared_ptr<clang::driver::InputArgList>{
                 optTable.ParseArgs(
                     args.data(), args.data() + args.size(),
                     missingArgIndex, missingArgCount
                 )
            };

            if (command->getSource().getKind() == clang::driver::Action::CompileJobClass ||
                command->getSource().getKind() == clang::driver::Action::AssembleJobClass) {
                toPut.operation = command::COMPILE;
            } else if(command->getSource().getKind() == clang::driver::Action::LinkJobClass) {
                toPut.operation = command::LINK;
            } else toPut.operation = command::NOP;

            ret.push_back(toPut);
        } else {
            BYE_BYE(decltype(ret), "Encountered a non-Command Job");
        }
    }

    return std::move(ret);
}

interviewer::status interviewer::run() const {
    using borealis::util::streams::error;

    if (!pimpl->theCompilation) {
        errs() << error("Creating compilation object failed") << endl;
        return status::FAILURE;
    }

    const clang::driver::Command* FailingCommand;
    if (pimpl->theDriver->ExecuteCompilation(*pimpl->theCompilation, FailingCommand) < 0) {
        pimpl->theDriver->generateCompilationDiagnostics(*pimpl->theCompilation, FailingCommand);
        errs() << error("Compilation failed") << endl;
        return status::FAILURE;
    }

    return status::SUCCESS;
}

} // namespace driver
} // namespace borealis

#include "Util/unmacros.h"
