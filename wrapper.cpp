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
#include <llvm/Support/PluginLoader.h>
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
#include "Logging/logger.hpp"
#include "Passes/Misc/PrinterPasses.h"
#include "Passes/Util/DataProvider.hpp"
#include "Util/util.h"

#include "wrapper.h"

struct program_args {
    std::vector<std::string> passes;
    std::vector<std::string> libs;
    std::vector<const char*> opt;
    const char** opt_argv;
    int opt_argc;
    std::string config;
    std::vector<const char*> compiler;
    const char** compiler_argv;
    int compiler_argc;

    std::string clang;

    program_args(int argc, const char** argv) :
            opt { "wrapper" },
            clang { "/usr/bin/clang-3.1" } {
        using borealis::util::view;

        for (llvm::StringRef arg : view(argv+1, argv+argc)) {
            if (arg.startswith("-pass")) passes.push_back(arg.drop_front(5).str());
            else if (arg.startswith("-lib")) libs.push_back(arg.drop_front(4).str());
            else if (arg.startswith("-opt")) opt.push_back(arg.drop_front(4).data());
            else if (arg.startswith("-cfg")) config = (arg.drop_front(4).str());
            else if (arg.startswith("-clang")) clang = (arg.drop_front(6).str());
            else compiler.push_back(arg.data());
        }
        // this is generally fucked up
        compiler.push_back("-I/usr/lib/clang/" CLANG_VERSION_STRING "/include");
        compiler_argv = compiler.data();
        compiler_argc = compiler.size();
        opt_argv = opt.data();
        opt_argc = opt.size();
    }

    template<class InputIterator>
    program_args& addOptArguments(InputIterator from, InputIterator to) {
        using borealis::util::view;

        for (const std::string& arg : view(from, to)) opt.push_back(arg.c_str());
        opt_argv = opt.data();
        opt_argc = opt.size();
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& ost, const program_args& args) {
        using std::endl;
        ost << "passes: " << args.passes << endl;
        ost << "libs: " << args.libs << endl;
        ost << "opt: " << args.opt << endl;
        ost << "config: " << args.config << endl;
        ost << "compiler: " << args.compiler << endl;
        ost << "clang: " << args.clang << endl;
        return ost;
    }
};

int main(int argc, const char** argv) {

    GOOGLE_PROTOBUF_VERIFY_VERSION;
    atexit(google::protobuf::ShutdownProtobufLibrary);

    using namespace clang;
    using namespace llvm;

    using namespace borealis::config;

    using borealis::comments::GatherCommentsAction;
    using borealis::endl;
    using borealis::util::streams::error;

    constexpr auto loggingDomain = "wrapper";

    auto infos = []{ return borealis::logging::infosFor(loggingDomain); };
    auto errs = []{ return borealis::logging::errsFor(loggingDomain); };

    const std::vector<std::string> empty;

    program_args args(argc, argv);

    AppConfiguration::initialize(args.config.empty() ? "wrapper.conf" : args.config.c_str());

    StringConfigEntry logFile("logging", "ini");
    StringConfigEntry z3log("logging", "z3log");

    MultiConfigEntry opt_args("opt", "load");

    MultiConfigEntry prePasses("passes", "pre");
    MultiConfigEntry postPasses("passes", "post");
    MultiConfigEntry libs("libs", "load");

    for (const auto& op : logFile) {
        borealis::logging::configureLoggingFacility(op);
    }

    for (const auto& op : z3log) {
        borealis::logging::configureZ3Log(op);
    }

    // args to supply to opt
    args.addOptArguments(opt_args.begin(), opt_args.end());

    cl::ParseCommandLineOptions(args.opt_argc, args.opt_argv);

    {
        borealis::logging::log_entry out(infos());
        out << endl;
        for (const auto& arg : args.compiler) {
            out << arg << " ";
        }
        out << endl;
    }

    DiagnosticOptions DiagOpts;
    auto* tdp = new TextDiagnosticPrinter(llvm::errs(), DiagOpts);
    auto diags = CompilerInstance::createDiagnostics(DiagOpts, args.compiler_argc, args.compiler_argv, tdp);

    // first things first
    // fall-through to the regular clang
    if (args.clang != "" &&
        args.clang != "no" &&
        args.clang != "skip") {

        clang::driver::Driver Driver(args.clang, llvm::sys::getDefaultTargetTriple(), "a.out", true, *diags);

        std::vector<const char*> compilationArgs;
        compilationArgs.reserve(1 + args.compiler.size());
        compilationArgs.push_back(args.clang.c_str());
        compilationArgs.insert(compilationArgs.end(), args.compiler.begin(), args.compiler.end());

        auto Compilation = borealis::util::uniq(Driver.BuildCompilation(compilationArgs));
        if (!Compilation) { errs() << error("Fucked up, sorry :(") << endl; return borealis::E_CLANG_INVOKE; }

        const clang::driver::Command* FailingCommand;
        if (Driver.ExecuteCompilation(*Compilation, FailingCommand) < 0) {
            Driver.generateCompilationDiagnostics(*Compilation, FailingCommand);
            errs() << error("Fucked up, sorry :(") << endl; return borealis::E_CLANG_INVOKE;
        }
    }

    // setup and execute borealis stuff
    CompilerInstance Clang;
    Clang.setDiagnostics(diags.getPtr());
    Clang.setInvocation(createInvocationFromCommandLine(args.compiler, diags));
    if (!Clang.hasInvocation()) { errs() << error("Fucked up, sorry :(") << endl; return borealis::E_ILLEGAL_COMPILER_OPTIONS; }

    // print the argument list from the "borealis" compiler invocation
    std::vector<std::string> argsFromInvocation;
    Clang.getInvocation().toArgs(argsFromInvocation);

    {
        borealis::logging::log_entry out(infos());
        out << "clang ";
        for (const auto& arg : argsFromInvocation) {
            out << arg << " ";
        }
        out << endl;
    }

    // maximize debug metadata
    Clang.getCodeGenOpts().EmitDeclMetadata = true;
    Clang.getCodeGenOpts().DebugInfo = true;

    GatherCommentsAction Proc;
    if (!Clang.ExecuteAction(Proc)) { errs() << error("Fucked up, sorry :(") << endl; return borealis::E_GATHER_COMMENTS; }

    EmitLLVMOnlyAction Act;
    if (!Clang.ExecuteAction(Act)) { errs() << error("Fucked up, sorry :(") << endl; return borealis::E_EMIT_LLVM; }

    std::unique_ptr<llvm::Module> module_ptr(Act.takeModule());
    auto& module = *module_ptr;

    PassManager pm;
    // explicitly schedule TargetData pass as the first pass to run;
    // note that M.get() gets a pointer or reference to the module to analyze
    pm.add(new TargetData(module_ptr.get()));

    std::vector<StringRef> passes2run;
    for (StringRef p : prePasses) {
        passes2run.push_back(p);
    }
    passes2run.insert(passes2run.end(), args.passes.begin(), args.passes.end());
    for (StringRef p : postPasses) {
        passes2run.push_back(p);
    }

    std::vector<StringRef> libs2load;
    for (StringRef l : libs) {
        libs2load.push_back(l);
    }
    libs2load.insert(libs2load.end(), args.libs.begin(), args.libs.end());
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

    PluginLoader pl;

    for (const auto& lib : libs2load) {
        pl = lib;
    }

    // initialize passes
    auto& reg = *PassRegistry::getPassRegistry();
    initializeCore(reg);
    initializeScalarOpts(reg);
    initializeIPO(reg);
    initializeAnalysis(reg);
    initializeIPA(reg);
    initializeTransformUtils(reg);
    initializeInstCombine(reg);
    initializeInstrumentation(reg);
    initializeTarget(reg);

    using borealis::provideAsPass;
    pm.add(provideAsPass(&Proc));
    pm.add(provideAsPass(&Clang.getSourceManager()));

    using borealis::createPrinterFor;
    if (!passes2run.empty()) {
        for (StringRef pass : passes2run) {
            bool isPrinterPass = pass.endswith("-printer");
            StringRef passName = isPrinterPass ? pass.drop_back(8) : pass;

            auto* passInfo = reg.getPassInfo(passName);
            if (passInfo == nullptr) {
                errs() << "Pass " << passName << " cannot be found" << endl;
                continue;
            }

            infos() << pass << ": " << passInfo->getPassName() << endl;

            if (auto* ctor = passInfo->getNormalCtor()) {
                auto thePass = ctor();
                pm.add(thePass);
                if (isPrinterPass) {
                    pm.add(createPrinterFor(passInfo, thePass));
                }
            } else {
                errs() << "Could not create pass " << passInfo->getPassName() << endl;
            }
        }

        pm.run(module);

        std::string error;
        if (verifyModule(module, ReturnStatusAction, &error)) {
            errs() << "Module errors detected: " << error << endl;
        }
    }

    return borealis::OK;
}
