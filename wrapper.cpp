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

struct args {

    std::string config;
    std::vector<std::string> compiler;
    borealis::config::Config::Overrides overrides;

    args(int argc, const char** argv) {
        using borealis::util::view;

        std::vector<std::string> args;

        for (llvm::StringRef arg : view(argv+1, argv+argc)) {
            if (arg.startswith("---config:")) config = arg.drop_front(10);
            if (arg.startswith("---")) args.push_back(arg.drop_front(3));
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

    args args(argc, argv);

    AppConfiguration::initialize(args.config.c_str(), args.overrides);

    StringConfigEntry logFile("logging", "ini");
    StringConfigEntry z3log("logging", "z3log");

    auto opt = MultiConfigEntry("opt", "load").get();

    auto prePasses = MultiConfigEntry("passes", "pre").get();
    auto inPasses = MultiConfigEntry("passes", "in").get();
    auto postPasses = MultiConfigEntry("passes", "post").get();
    auto libs = MultiConfigEntry("libs", "load").get();

    auto clangExec = StringConfigEntry("run", "clangExec").get("/usr/bin/clang");
    auto skipClang = BoolConfigEntry("run", "skipClangDriver").get(false);

    for (const auto& op : logFile) {
        borealis::logging::configureLoggingFacility(op);
    }

    for (const auto& op : z3log) {
        borealis::logging::configureZ3Log(op);
    }

    auto opt_ = charify(opt);
    cl::ParseCommandLineOptions(opt_.size(), opt_.data());

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
    auto* tdp = new TextDiagnosticPrinter(llvm::errs(), DiagOpts);
    auto diags = CompilerInstance::createDiagnostics(DiagOpts, compiler_args.size(), compiler_args.data(), tdp);

    // first things first
    // fall-through to the regular clang
    if (!skipClang) {

        clang::driver::Driver Driver(clangExec, llvm::sys::getDefaultTargetTriple(), "a.out", true, *diags);

        std::vector<const char*> compilation_args;
        compilation_args.reserve(1 + compiler_args.size());
        compilation_args.push_back(clangExec.c_str());
        compilation_args.insert(compilation_args.end(), compiler_args.begin(), compiler_args.end());

        auto Compilation = borealis::util::uniq(Driver.BuildCompilation(compilation_args));
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
    Clang.setInvocation(createInvocationFromCommandLine(compiler_args, diags));
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

    std::string err;
    if (verifyModule(module, ReturnStatusAction, &err)) {
        errs() << "Module errors detected: " << err << endl;
    }

    return borealis::OK;
}
