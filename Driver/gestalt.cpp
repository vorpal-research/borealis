/*
 * gestalt.cpp
 *
 *  Created on: Aug 27, 2013
 *      Author: belyaev
 */

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
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticBuffer.h>
#include <clang/Frontend/Utils.h>
#include <clang/FrontendTool/Utils.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/Parser.h>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/PassManager.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Program.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SystemUtils.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Target/TargetMachine.h>

#include <google/protobuf/stubs/common.h>

#include "Actions/GatherCommentsAction.h"
#include "Config/config.h"
#include "Codegen/DiagnosticLogger.h"
#include "Driver/cl.h"
#include "Driver/clang_pipeline.h"
#include "Driver/gestalt.h"
#include "Driver/interviewer.h"
#include "Driver/llvm_pipeline.h"
#include "Driver/plugin_loader.h"
#include "Logging/logger.hpp"
#include "Passes/Misc/PrinterPasses.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/util.h"

namespace borealis {
namespace driver {

int gestalt::main(int argc, const char** argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    atexit(google::protobuf::ShutdownProtobufLibrary);

    borealis::util::initFilePaths(argv);

    // XXX: sometimes this is causing "pure virtual method called" on sayonara
    // atexit(llvm::llvm_shutdown);

    using namespace clang;
    using namespace llvm;

    using namespace borealis::config;

    using borealis::endl;

    const std::vector<std::string> empty;

    CommandLine args(argc, argv);
    std::string configPath = "wrapper.conf";
    std::string defaultLogIni = "log.ini";

    AppConfiguration::initialize(
        new CommandLineConfigSource{ args.suffixes("---").stlRep() },
        new FileConfigSource{ args.suffixes("---config:").single(util::getFilePathIfExists(configPath).c_str()) }
    );

    CommandLine opt = CommandLine("wrapper") +
        args.suffixes("---opt:") +
        MultiConfigEntry("opt", "load").get();
    llvm::cl::ParseCommandLineOptions(opt.argc(), opt.argv());

    StringConfigEntry logFile("logging", "ini");
    StringConfigEntry z3log("logging", "z3log");


    borealis::logging::configureLoggingFacility(
        util::getFilePathIfExists(logFile.get().getOrElse(defaultLogIni))
    );

    for (const auto& op : z3log) {
        borealis::logging::configureZ3Log(util::getFilePathIfExists(op));
    }

    auto prePasses = MultiConfigEntry("passes", "pre").get();
    auto inPasses = MultiConfigEntry("passes", "in").get();
    auto postPasses = MultiConfigEntry("passes", "post").get();
    auto libs = MultiConfigEntry("libs", "load").get();
    auto skipClang = BoolConfigEntry("run", "skipClangDriver").get(false);

    CommandLine compilerArgs = args.tail().unprefix("---");

    infos() << "Invoking clang with: " << compilerArgs << endl;

    auto diagOpts = llvm::IntrusiveRefCntPtr<DiagnosticOptions>(new DiagnosticOptions{});
    diagOpts->ShowCarets = false;

    auto diagBuffer = borealis::util::uniq(new DiagnosticLogger(*this));
    auto diags = CompilerInstance::createDiagnostics(diagOpts.get(), diagBuffer.get(), /* ownClient = */false);

    // first things first
    // fall-through to the regular clang

    auto nativeClangCfg = borealis::config::StringConfigEntry("run", "clangExec");

    interviewer nativeClang{ "clang", compilerArgs.data(), diags, nativeClangCfg };
    nativeClang.assignLogger(*this);

    auto compileCommands = nativeClang.getCompileCommands();

    if (!skipClang) if (nativeClang.run() == interviewer::status::FAILURE) return E_CLANG_INVOKE;

    // prep for borealis business
    // compile sources to llvm::Module

    if (compileCommands.empty()) return E_ILLEGAL_COMPILER_OPTIONS;

    clang_pipeline clang { "clang", diags };


    clang.assignLogger(*this);
    clang.invoke(compileCommands);

    auto annotatedModule = clang.result();

    if (!annotatedModule) return OK;
    // TODO: Distinct between cases when we fucked up and when
    //       we were supposed to fuck up

    // collect passes

    auto module_ptr = annotatedModule->module;

    std::vector<StringRef> passes2run;
    passes2run.insert(passes2run.end(), prePasses.begin(), prePasses.end());
    passes2run.insert(passes2run.end(), inPasses.begin(), inPasses.end());
    passes2run.insert(passes2run.end(), postPasses.begin(), postPasses.end());

    std::vector<StringRef> libs2load;
    libs2load.insert(libs2load.end(), libs.begin(), libs.end());

    {
        borealis::logging::log_entry out(infos());
        out << "Passes:" << endl;
        if (passes2run.empty()) out << "  " << "None" << endl;
        for (const auto& pass : passes2run) {
            out << "  " << pass << endl;
        }
    }

    {
        borealis::logging::log_entry out(infos());
        out << "Dynamic libraries:" << endl;
        if (libs2load.empty()) out << "  " << "None" << endl;
        for (const auto& lib : libs2load) {
            out << "  " << lib << endl;
        }
    }

    // load .so plugins

    plugin_loader pl;
    pl.assignLogger(*this);
    for (const auto& lib : libs2load) {
        pl.add(util::getFilePathIfExists(lib.str()));
    }
    pl.run();

    // run llvm passes

    llvm_pipeline llvm { module_ptr };
    llvm.assignLogger(*this);

    llvm.add(*annotatedModule->annotations);
    clang::FileManager files{ FileSystemOptions() };
    llvm.add(files);
    for (StringRef pass : passes2run) {
        llvm.add(pass.str());
    }

    llvm.run();

    // verify we didn't screw up the module structure

    std::string err;
    llvm::raw_string_ostream rso{err};
    if (verifyModule(*module_ptr, &rso)) {
        errs() << "Module errors detected: " << err << endl;
    }

    return OK;
}

gestalt::~gestalt() {}

} /* namespace driver */
} /* namespace borealis */
