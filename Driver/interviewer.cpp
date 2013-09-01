/*
 * interviewer.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <clang/Basic/Diagnostic.h>
#include <clang/Driver/Arg.h>
#include <clang/Driver/ArgList.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Option.h>
#include <clang/Driver/Options.h>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/Host.h> // getDefaultTargetTriple

#include "Config/config.h"
#include "Driver/interviewer.h"
#include "Logging/logger.hpp"
#include "Util/util.h"

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

    // FIXME: Huh?
    // if (pimpl->theCompilation->getArgs().hasArg(clang::driver::options::OPT_c)) {
    //     errs() << "Hello!" << endl;
    // }

    for (const auto& inp :
            borealis::util::view(pimpl->theCompilation->getArgs().filtered_begin(clang::driver::options::OPT_INPUT),
                                 pimpl->theCompilation->getArgs().filtered_end())
    ) {
        dbgs() << inp->getValue(pimpl->theCompilation->getArgs()) << endl;
    }
};

util::option_ref<const clang::driver::DerivedArgList> interviewer::getRealArgs() const {
    using borealis::util::streams::error;

    if (!pimpl->theCompilation) {
        errs() << error("Fucked up, sorry :(") << endl;
        return util::nothing();
    }

    return util::justRef(pimpl->theCompilation->getArgs());
}

interviewer::status interviewer::run() const {
    using borealis::util::streams::error;

    if (!pimpl->theCompilation) {
        errs() << error("Fucked up, sorry :(") << endl;
        return status::FAILURE;
    }

    const clang::driver::Command* FailingCommand;
    if (pimpl->theDriver->ExecuteCompilation(*pimpl->theCompilation, FailingCommand) < 0) {
        pimpl->theDriver->generateCompilationDiagnostics(*pimpl->theCompilation, FailingCommand);
        errs() << error("Fucked up, sorry :(") << endl;
        return status::FAILURE;
    }

    return status::SUCCESS;
}

} // namespace driver
} // namespace borealis
