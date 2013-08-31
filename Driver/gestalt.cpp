/*
 * gestalt.cpp
 *
 *  Created on: Aug 27, 2013
 *      Author: belyaev
 */

#include "Driver/gestalt.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/Version.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticBuffer.h>
#include <clang/Frontend/Utils.h>
#include <clang/FrontendTool/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/Parser.h>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/DebugInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/BasicBlock.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/CallGraphSCCPass.h>
#include <llvm/Function.h>
#include <llvm/Instruction.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/LinkAllVMCore.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/PassNameParser.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SystemUtils.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Target/TargetMachine.h>

#include <google/protobuf/stubs/common.h>

#include "Actions/comments.h"
#include "Config/config.h"
#include "Codegen/DiagnosticLogger.h"
#include "Driver/clang_pipeline.h"
#include "Driver/interviewer.h"
#include "Driver/llvm_pipeline.h"
#include "Driver/plugin_loader.h"
#include "Driver/cl.h"
#include "Logging/logger.hpp"
#include "Passes/Misc/PrinterPasses.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/util.h"

namespace borealis {
namespace driver {

int gestalt::main(int argc, const char** argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    atexit(google::protobuf::ShutdownProtobufLibrary);

    using namespace clang;
    using namespace llvm;

    using namespace borealis::config;

    using borealis::comments::GatherCommentsAction;
    using borealis::endl;
    using borealis::util::streams::error;

    const std::vector<std::string> empty;

    borealis::driver::CommandLine args(argc, argv);

    AppConfiguration::initialize(
        new borealis::config::CommandLineConfigSource{ args.suffixes("---").stlRep() },
        new borealis::config::FileConfigSource{ args.suffixes("---config:").single("wrapper.conf") }
    );

    borealis::driver::CommandLine opt =
            args.suffixes("---opt:") +
            MultiConfigEntry("opt", "load").get();

    llvm::cl::ParseCommandLineOptions(opt.argc(), opt.argv());

    StringConfigEntry logFile("logging", "ini");
    StringConfigEntry z3log("logging", "z3log");

    auto prePasses = MultiConfigEntry("passes", "pre").get();
    auto inPasses = MultiConfigEntry("passes", "in").get();
    auto postPasses = MultiConfigEntry("passes", "post").get();
    auto libs = MultiConfigEntry("libs", "load").get();

    auto skipClang = BoolConfigEntry("run", "skipClangDriver").get(false);

    for (const auto& op : logFile) {
        borealis::logging::configureLoggingFacility(op);
    }
    for (const auto& op : z3log) {
        borealis::logging::configureZ3Log(op);
    }

    CommandLine compilerArgs = args.tail().unprefix("---").
            push_back("-I/usr/lib/clang/" CLANG_VERSION_STRING "/include");

    infos() << "Invoking clang with: " << compilerArgs << endl;

    DiagnosticOptions DiagOpts;
    DiagOpts.ShowCarets = 0;

    auto diagBuffer = borealis::util::uniq(new DiagnosticLogger(*this));

    auto diags = CompilerInstance::createDiagnostics(DiagOpts,
            compilerArgs.argc(), compilerArgs.argv(),
            diagBuffer.get(), /* ownClient = */false, /* cloneClient = */false);

    auto nativeClangCfg = borealis::config::StringConfigEntry("run", "clangExec");
    interviewer nativeClang{ "clang", compilerArgs.data(), diags, nativeClangCfg };
    nativeClang.assignLogger(*this);

    //infos() << "Clang native arguments: " << nativeClang.getRealArgs() << endl;

    // setup and execute borealis stuff
    clang_pipeline clang { "clang", compilerArgs.data(), diags };
    clang.assignLogger(*this);

    plugin_loader pl;
    pl.assignLogger(*this);

    // first things first
    // fall-through to the regular clang
    if (!skipClang) {
        if(nativeClang.run() == interviewer::status::FAILURE) return E_CLANG_INVOKE;
    }

    borealis::driver::CommandLine clangRealArgs = clang.getInvocationArgs();
    if(clangRealArgs.empty()) return E_ILLEGAL_COMPILER_OPTIONS;

    infos() << "Clang lib arguments: " << clangRealArgs << endl;

    GatherCommentsAction gather_comments;
    EmitLLVMOnlyAction compile_to_llvm;

    clang.add(gather_comments);
    clang.add(compile_to_llvm);

    if (clang.run() == clang_pipeline::status::FAILURE) {
        return E_GATHER_COMMENTS;
    }

    std::shared_ptr<llvm::Module> module_ptr(compile_to_llvm.takeModule());

    std::vector<StringRef> passes2run;
    passes2run.insert(passes2run.end(), prePasses.begin(), prePasses.end());
    passes2run.insert(passes2run.end(), inPasses.begin(), inPasses.end());
    passes2run.insert(passes2run.end(), postPasses.begin(), postPasses.end());

    std::vector<StringRef> libs2load;
    libs2load.insert(libs2load.end(), libs.begin(), libs.end());

    {
        borealis::logging::log_entry out(infos());
        out << "Passes:" << endl;
        if (passes2run.empty()) out << "  " << error("None") << endl;
        for (const auto& pass : passes2run) {
            out << "  " << pass << endl;
        }
    }

    {
        borealis::logging::log_entry out(infos());
        out << "Dynamic libraries:" << endl;
        if (libs2load.empty()) out << "  " << error("None") << endl;
        for (const auto& lib : libs2load) {
            out << "  " << lib << endl;
        }
    }

    for (const auto& lib : libs2load) {
        pl.add(lib.str());
    }

    pl.run();

    llvm_pipeline llvm { module_ptr };
    llvm.assignLogger(*this);

    llvm.add(gather_comments);
    llvm.add(clang.getSourceManager());
    for (StringRef pass : passes2run) {
        llvm.add(pass.str());
    }

    llvm.run();

    std::string err;
    if (verifyModule(*module_ptr, ReturnStatusAction, &err)) {
        errs() << "Module errors detected: " << err << endl;
    }

    return OK;
}

gestalt::~gestalt() {}

} /* namespace driver */
} /* namespace borealis */