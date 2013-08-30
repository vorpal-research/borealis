/*
 * wrapper.cpp
 *
 *  Created on: Nov 1, 2012
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
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
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
#include "Driver/clang_pipeline.h"
#include "Driver/interviewer.h"
#include "Driver/llvm_pipeline.h"
#include "Driver/plugin_loader.h"
#include "Logging/logger.hpp"
#include "Passes/Misc/PrinterPasses.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/util.h"

#include "wrapper.h"

struct args {

    std::string config;
    std::vector<std::string> compiler;
    std::vector<std::string> opt;
    borealis::config::Config::Overrides overrides;

    args(int argc, const char** argv) : opt{ "wrapper" } {
        using borealis::util::view;

        std::vector<std::string> args;

        for (llvm::StringRef arg : view(argv+1, argv+argc)) {
            if (arg.startswith("---config:")) config = arg.drop_front(10);
            else if (arg.startswith("---opt:")) opt.push_back(arg.drop_front(7));
            else if (arg.startswith("---")) args.push_back(arg.drop_front(3));
            else compiler.push_back(arg);
        }

        for (llvm::StringRef arg : args) {

            llvm::SmallVector<llvm::StringRef, 3> elems;
            arg.split(elems, ":", 3);
            if (elems.size() != 3) continue;

            auto& section = elems[0];
            auto& key = elems[1];
            auto& value = elems[2];
            overrides[section][key].push_back(value);
        }

        if (config.empty()) config = "wrapper.conf";
        // This is generally fucked up
        compiler.push_back("-I/usr/lib/clang/" CLANG_VERSION_STRING "/include");
    }

    void addOptArgs(const std::vector<std::string>& extra) {
        opt.insert(opt.end(), extra.begin(), extra.end());
    }
};

std::vector<const char*> charify(const std::vector<std::string>& vec) {
    std::vector<const char*> res;
    res.reserve(vec.size());
    std::transform(vec.begin(), vec.end(), std::back_inserter(res),
        [](const std::string& e) { return e.c_str(); }
    );
    return res;
}

int main(int argc, const char** argv) {

    GOOGLE_PROTOBUF_VERIFY_VERSION;
    atexit(google::protobuf::ShutdownProtobufLibrary);

    atexit(llvm::llvm_shutdown);

    using namespace clang;
    using namespace llvm;

    using namespace borealis::config;

    using borealis::comments::GatherCommentsAction;
    using borealis::endl;
    using borealis::util::streams::error;

    using borealis::driver::interviewer;
    using borealis::driver::clang_pipeline;
    using borealis::driver::llvm_pipeline;
    using borealis::driver::plugin_loader;

    constexpr auto loggingDomain = "wrapper";

    auto infos = []{ return borealis::logging::infosFor(loggingDomain); };
    auto errs = []{ return borealis::logging::errsFor(loggingDomain); };

    const std::vector<std::string> empty;

    args args(argc, argv);

    AppConfiguration::initialize(args.config.c_str(), args.overrides);

    args.addOptArgs(MultiConfigEntry("opt", "load").get());

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

    auto opt = charify(args.opt);
    cl::ParseCommandLineOptions(opt.size(), opt.data());

    {
        borealis::logging::log_entry out(infos());
        out << endl;
        for (const auto& arg : args.compiler) {
            out << arg << " ";
        }
        out << endl;
    }

    auto compiler_args = charify(args.compiler);

    DiagnosticOptions DiagOpts;
    auto wrapper_errs = errs();
    borealis::util::streams::llvm_stream_wrapper<decltype(errs())> llvmerrs(wrapper_errs);
    auto tdp = borealis::util::uniq(new TextDiagnosticPrinter(llvmerrs, DiagOpts));
    auto diags = CompilerInstance::createDiagnostics(DiagOpts, compiler_args.size(), compiler_args.data(), tdp.release());

    auto nativeClangCfg = borealis::config::StringConfigEntry("run", "clangExec");
    interviewer nativeClang{ "clang", compiler_args, diags, nativeClangCfg };

    // first things first
    // fall-through to the regular clang
    if (!skipClang) {
        if(nativeClang.run() == interviewer::status::FAILURE) return borealis::E_CLANG_INVOKE;
    }

    // setup and execute borealis stuff
    clang_pipeline clang { "clang", compiler_args, diags };

    auto clangRealArgs = clang.getInvocationArgs();
    if(clangRealArgs.empty()) return borealis::E_ILLEGAL_COMPILER_OPTIONS;

    infos() << clangRealArgs << endl;

    GatherCommentsAction gather_comments;
    EmitLLVMOnlyAction compile_to_llvm;

    clang.add(gather_comments);
    clang.add(compile_to_llvm);

    if (clang.run() == clang_pipeline::status::FAILURE) {
        return borealis::E_GATHER_COMMENTS;
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

    plugin_loader pl;

    for (const auto& lib : libs2load) {
        pl.add(lib.str());
    }

    pl.run();

    llvm_pipeline llvm { module_ptr };

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

    return borealis::OK;
}

