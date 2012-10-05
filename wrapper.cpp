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
#include <llvm/Support/Host.h>
#include <llvm/Support/PassNameParser.h>
#include <llvm/Support/PluginLoader.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/PassNameParser.h>
#include <llvm/Support/PluginLoader.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SystemUtils.h>
#include <llvm/Support/ToolOutputFile.h>

#include "comments.h"
#include "util.h"

using namespace std;
using llvm::raw_ostream;

using namespace borealis;
using util::for_each;
using util::streams::error;
using util::streams::endl;

template<class T>
void print(const T& val) {
	llvm::errs() << val << " ";
}



int main(int argc, const char** argv)
{
	using namespace clang;
	using namespace llvm;

	// arguments to pass to the clang front-end
	vector<const char *> args(argv,argv+argc);

	args.erase(args.begin());
	args.push_back("-g");

	// this is generally fucked up
	args.push_back("-I/usr/lib/clang/" CLANG_VERSION_STRING "/include");

	auto cargs = args; // args we supply to the compiler itself, ignoring those used by us
	auto newend =
			std::remove_if(cargs.begin(), cargs.end(),
					[](const StringRef& ref) {return ref.startswith("-pass") || ref.startswith("-load");});
	cargs.erase(newend, cargs.end());

	for_each(cargs, print<StringRef>);
	errs() << endl;

    DiagnosticOptions DiagOpts;
    auto diags = CompilerInstance::createDiagnostics(DiagOpts, cargs.size(), &*cargs.begin());

	OwningPtr<CompilerInvocation> invoke(createInvocationFromCommandLine(cargs, diags));



	// Print the argument list from the "real" compiler invocation
	errs() << ("clang ");
	vector<string> argsFromInvocation;
	invoke->toArgs(argsFromInvocation);
	for (auto i = argsFromInvocation.begin(); i != argsFromInvocation.end(); ++i)
		errs() << (*i) << " ";
	errs() << endl;

	clang::CompilerInstance Clang;
	Clang.setInvocation(invoke.take());
	auto& stillInvoke = Clang.getInvocation();

	auto& to = stillInvoke.getTargetOpts();
	auto pti = TargetInfo::CreateTargetInfo(*diags, to);

	Clang.setTarget(pti);
	Clang.setDiagnostics(diags.getPtr());

	// Create an action and make the compiler instance carry it out
	OwningPtr<comments::GatherCommentsAction> Proc(new comments::GatherCommentsAction());
	if(!Clang.ExecuteAction(*Proc)){ errs() << error("Fucked up, sorry :(") << endl; return 1; }

	OwningPtr<CodeGenAction> Act(new EmitLLVMOnlyAction());
	if(!Clang.ExecuteAction(*Act)){ errs() << error("Fucked up, sorry :(") << endl; return 1; }

	auto& module = *Act->takeModule();

	PassManager pm;
	// Explicitly schedule TargetData pass as the first pass to run;
	// note that M.get() gets a pointer or reference to the module to analyze
	pm.add(new TargetData(&module));

	vector<StringRef> passes2run;

	vector<StringRef> libs2load;

	for_each(args,
			[&libs2load](const StringRef& mes) {
				if( mes.startswith("-load") ) libs2load.push_back(mes.drop_front(5));
			});

	for_each(args,
			[&passes2run](const StringRef& mes) {
				if( mes.startswith("-pass") ) passes2run.push_back(mes.drop_front(5));
			});

	errs() << "Passes:" << endl;
	if (passes2run.empty())
		errs() << error("None") << endl;
	for_each(passes2run, [](const StringRef& pass) {
		errs().indent(2);
		errs() << pass << endl;
	});

	errs() << "Dynamic libraries:" << endl;
	if (libs2load.empty())
		errs() << error("None") << endl;
	for_each(libs2load, [](const StringRef& lib) {
		errs().indent(2);
		errs() << lib << endl;
	});

	PluginLoader pl;

	for_each(libs2load, [&](const StringRef& lib) {
		pl = lib;
	});

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

	if (!passes2run.empty()) {
		for_each(passes2run,
				[&](const StringRef& pass) {
					auto passInfo = reg.getPassInfo(pass);

					if(passInfo == nullptr) {
						errs () << "Pass " << pass << " cannot be found" << endl;
						return;
					}

					errs() << pass << ":" << passInfo->getPassName() << endl;

					if(passInfo->getNormalCtor()) {
						auto thePass = passInfo->getNormalCtor()();
						pm.add(thePass);
					} else {
						errs() << "Could not create pass " << passInfo->getPassName() << endl;
					}
				});

		pm.run(module);

		verifyModule(module, PrintMessageAction);

		errs().resetColor();
	}

	return 0;
}
