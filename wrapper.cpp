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
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Frontend/Utils.h>
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

#include "Actions/comments.h"
#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Passes/DataProvider.hpp"
#include "Passes/PrinterPasses.h"
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

    program_args(int argc, const char** argv):
        passes(), libs(),
        opt{"wrapper"}, opt_argv(), opt_argc(),
        config(),
        compiler(), compiler_argv(nullptr), compiler_argc(0) {
        using borealis::util::view;
        for(llvm::StringRef arg : view(argv+1, argv+argc)) {
            if(arg.startswith("-pass")) passes.push_back(arg.drop_front(5).str());
            else if(arg.startswith("-lib")) passes.push_back(arg.drop_front(4).str());
            else if(arg.startswith("-opt")) passes.push_back(arg.drop_front(4).data());
            else if(arg.startswith("-cfg")) config = (arg.drop_front(4).str());
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
    program_args& addOptArgument(InputIterator from, InputIterator to) {
        using borealis::util::view;

        for(const std::string& arg: view(from, to)) opt.push_back(arg.c_str());
        opt_argv = opt.data();
        opt_argc = opt.size();
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& ost, const program_args& args) {
        ost << "passes: " << args.passes << std::endl;
        ost << "libs: " << args.libs << std::endl;
        ost << "opt: " << args.opt << std::endl;
        ost << "config: " << args.config << std::endl;
        ost << "compiler: " << args.compiler << std::endl;
        return ost;
    }
};

int main(int argc, const char** argv)
{
    using namespace clang;
    using namespace llvm;

    using borealis::comments::GatherCommentsAction;
    using borealis::config::Config;
    using borealis::endl;
    using borealis::logging::log_entry;
    using borealis::util::streams::error;

    constexpr auto loggingDomain = "wrapper";

    auto infos = [](){
        return borealis::logging::infosFor(loggingDomain);
    };

    auto errs = [](){
        return borealis::logging::errsFor(loggingDomain);
    };

    const std::vector<std::string> empty;

    program_args args(argc, argv);

    using namespace borealis::config;

    AppConfiguration::initialize(args.config.empty()? "wrapper.conf" : args.config.c_str());

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

    // arguments to pass to the clang front-end
    // args to supply to opt
    args.addOptArgument(opt_args.begin(), opt_args.end());

    llvm::cl::ParseCommandLineOptions(args.opt_argc, args.opt_argv);

    {
        auto out = infos();
        out << endl;
        for (const auto& arg : args.compiler) {
            out << arg << " ";
        }
        out << endl;
    }

    DiagnosticOptions DiagOpts;
    auto* tdp = new TextDiagnosticPrinter(llvm::errs(), DiagnosticOptions());
    auto diags = CompilerInstance::createDiagnostics(DiagOpts, args.compiler_argc, args.compiler_argv, tdp);

    std::unique_ptr<CompilerInvocation> invoke(createInvocationFromCommandLine(args.compiler, diags));

    // Print the argument list from the "real" compiler invocation
    std::vector<std::string> argsFromInvocation;
    invoke->toArgs(argsFromInvocation);

    {
        auto out = infos();
        out << "clang ";
        for (const auto& arg : argsFromInvocation) {
            out << arg << " ";
        }
        out << endl;
    }

    clang::CompilerInstance Clang;
    Clang.setInvocation(invoke.release());
    auto& stillInvoke = Clang.getInvocation();

    auto& to = stillInvoke.getTargetOpts();
    auto* pti = TargetInfo::CreateTargetInfo(*diags, to);

    Clang.setTarget(pti);
    Clang.setDiagnostics(diags.getPtr());
    // maximize debug metadata
    Clang.getCodeGenOpts().EmitDeclMetadata = true;
    Clang.getCodeGenOpts().DebugInfo = true;

    // Create an action and make the compiler instance carry it out
    GatherCommentsAction Proc;
    if (!Clang.ExecuteAction(Proc)) { errs() << error("Fucked up, sorry :(") << endl; return borealis::E_GATHER_COMMENTS; }

    EmitLLVMOnlyAction Act;
    if (!Clang.ExecuteAction(Act)) { errs() << error("Fucked up, sorry :(") << endl; return borealis::E_EMIT_LLVM; }

    std::unique_ptr<llvm::Module> module_ptr(Act.takeModule());
    auto& module = *module_ptr;

    PassManager pm;
    // Explicitly schedule TargetData pass as the first pass to run;
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
    libs2load.insert(passes2run.end(), args.libs.begin(), args.libs.end());
    {
        log_entry out(infos());
        out << "Passes:" << endl;
        if (passes2run.empty()) out << "  " << error("None") << endl;
        for (const auto& pass : passes2run) {
            out << "  " << pass << endl;
        }
    }

    {
        log_entry out(infos());
        out << "Dynamic libraries:" << endl;
        if (libs2load.empty()) out << "  " << error("None") << endl;
        for (const auto& lib: libs2load) {
            out << "  " << lib << endl;
        }
    }

    PluginLoader pl;

    for (const auto& lib : libs2load) {
        pl = lib;
    }

    // Initialize passes
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
                errs () << "Pass " << passName << " cannot be found" << endl;
                continue;
            }

            infos() << pass << ": " << passInfo->getPassName() << endl;

            if (passInfo->getNormalCtor()) {
                auto thePass = passInfo->getNormalCtor()();
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
