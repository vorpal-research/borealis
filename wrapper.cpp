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
#include "Util/util.h"

int main(int argc, const char** argv)
{
    using namespace clang;
    using namespace llvm;

    using borealis::comments::GatherCommentsAction;
    using borealis::config::Config;
    using borealis::errs;
    using borealis::endl;
    using borealis::infos;
    using borealis::logging::log_entry;
    using borealis::util::streams::error;

    Config cfg("wrapper.conf");
    auto logCFG = cfg.getValue<std::string>("logging", "ini");
    if(!logCFG.empty()) {
        borealis::logging::configureLoggingFacility(*logCFG.get());
    }

    // arguments to pass to the clang front-end
    std::vector<const char *> args(argv,argv+argc);

    args.erase(args.begin());
    args.push_back("-g");

    // this is generally fucked up
    args.push_back("-I/usr/lib/clang/" CLANG_VERSION_STRING "/include");

    auto cargs = args; // args we supply to the compiler itself, ignoring those used by us
    auto newend =
            std::remove_if(cargs.begin(), cargs.end(),
                    [](const StringRef& ref) {return ref.startswith("-pass") || ref.startswith("-load");});
    cargs.erase(newend, cargs.end());

    {
        auto out = borealis::logging::log_entry(infos());
        for(const auto& arg : cargs) {
            out << arg << " ";
        }
    }

    DiagnosticOptions DiagOpts;
    auto diags = CompilerInstance::createDiagnostics(DiagOpts, cargs.size(), &*cargs.begin());

    OwningPtr<CompilerInvocation> invoke(createInvocationFromCommandLine(cargs, diags));



    // Print the argument list from the "real" compiler invocation
    std::vector<std::string> argsFromInvocation;
    invoke->toArgs(argsFromInvocation);

    {
        log_entry out(infos());
        out << "clang ";
        for(const auto& arg : argsFromInvocation) {
            out << arg << " ";
        }
    }

    clang::CompilerInstance Clang;
    Clang.setInvocation(invoke.take());
    auto& stillInvoke = Clang.getInvocation();

    auto& to = stillInvoke.getTargetOpts();
    auto* pti = TargetInfo::CreateTargetInfo(*diags, to);

    Clang.setTarget(pti);
    Clang.setDiagnostics(diags.getPtr());

    // Create an action and make the compiler instance carry it out
    OwningPtr<GatherCommentsAction> Proc(new GatherCommentsAction());
    if(!Clang.ExecuteAction(*Proc)){ errs() << error("Fucked up, sorry :(") << endl; return 1; }

    OwningPtr<CodeGenAction> Act(new EmitLLVMOnlyAction());
    if(!Clang.ExecuteAction(*Act)){ errs() << error("Fucked up, sorry :(") << endl; return 1; }

    auto& module = *Act->takeModule();

    PassManager pm;
    // Explicitly schedule TargetData pass as the first pass to run;
    // note that M.get() gets a pointer or reference to the module to analyze
    pm.add(new TargetData(&module));


    const std::vector<std::string> empty;
    std::vector<StringRef> passes2run;

    auto prePasses = cfg.getValue< std::vector<std::string> >("passes", "pre").getOrElse(empty);
    for (StringRef p: prePasses) {
        passes2run.push_back(p);
    }
    for(StringRef p: args) {
        if( p.startswith("-pass") ) passes2run.push_back(p.drop_front(5));
    }

    auto postPasses = cfg.getValue< std::vector<std::string> >("passes", "post").getOrElse(empty);
    for (StringRef p: postPasses) {
        passes2run.push_back(p);
    }

    std::vector<StringRef> libs2load;

    auto libs = cfg.getValue< std::vector<std::string> >("libs", "load").getOrElse(empty);
    for (StringRef l: libs) {
        libs2load.push_back(l);
    }
    for (StringRef arg: args) {
        if( arg.startswith("-load") ) libs2load.push_back(arg.drop_front(5));
    }

    {
        log_entry out(infos());
        out << "Passes:" << endl;
        if (passes2run.empty())
            out << "  " << error("None") << endl;
        for (const auto& pass: passes2run) {
            out << "  ";
            out << pass << endl;
        }
    }


    {
        log_entry out(infos());
        out << "Dynamic libraries:" << endl;
        if (libs2load.empty())
            out << "  " << error("None") << endl;
        for (const auto& lib: libs2load) {
            out << "  ";
            out << lib << endl;
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
    pm.add(provideAsPass(Proc.get()));

    // args to supply to opt
    auto opt_args = cfg.getValue< std::vector<std::string> >("opt", "load").getOrElse(empty);

    size_t virt_argc = opt_args.size();
    {

        std::unique_ptr<const char*[]> virt_argv(new const char*[virt_argc]);
        for(size_t i = 0U; i < virt_argc; ++i) {
            virt_argv[i] = opt_args[i].c_str();
        }

        llvm::cl::ParseCommandLineOptions(virt_argc, virt_argv.get());
    }

    if (!passes2run.empty()) {
        for (const auto& pass : passes2run) {
            auto* passInfo = reg.getPassInfo(pass);

            if(passInfo == nullptr) {
                errs () << "Pass " << pass << " cannot be found" << endl;
                continue;
            }

            infos() << pass << ": " << passInfo->getPassName() << endl;

            if(passInfo->getNormalCtor()) {
                auto thePass = passInfo->getNormalCtor()();
                pm.add(thePass);
            } else {
                errs() << "Could not create pass " << passInfo->getPassName() << endl;
            }
        }

        pm.run(module);

        verifyModule(module, PrintMessageAction);

    }

    return 0;
}
