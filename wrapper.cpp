#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instruction.h>

#include "llvm/Support/Host.h"
#include "llvm/Support/PassNameParser.h"

#include "llvm/PassManager.h"

#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Parse/Parser.h"

#include "comments.h"
using comments::CommentKeeper;

using namespace std;

using llvm::raw_ostream;

#include "util.h"
using streams::endl;
using util::for_each;

int main(int argc, const char** argv) {
	using namespace llvm;
	using namespace clang;

	// Arguments to pass to the clang frontend
	vector<const char *> args(argv + 1, argv + argc);
	//args.push_back(inputPath.c_str());
	args.push_back("-g");
	auto cargs = args;
	auto newend = std::remove_if(cargs.begin(), cargs.end(), [](const StringRef& ref) { return ref.startswith("-pass"); } );
	cargs.resize(cargs.end() - newend,"");

	// The compiler invocation needs a DiagnosticsEngine so it can report problems
	TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(),
			clang::DiagnosticOptions());
	IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
	DiagnosticsEngine Diags(DiagID, DiagClient);

	// Create the compiler invocation
	llvm::OwningPtr<clang::CompilerInvocation> CI(
			new clang::CompilerInvocation);
	clang::CompilerInvocation::CreateFromArgs(*CI, &cargs[0],
			&cargs[0] + cargs.size(), Diags);

	// Print the argument list, which the compiler invocation has extended
	errs() << "clang ";
	vector<string> argsFromInvocation;
	CI->toArgs(argsFromInvocation);
	for (vector<string>::iterator i = argsFromInvocation.begin();
			i != argsFromInvocation.end(); ++i)
		errs() << ' ' << *i;
	errs() << endl;

	// Create the compiler instance
	// there must be a better way than creating two separate CompilerInstances
	CompilerInstance AnnotationCompiler;

	AnnotationCompiler.createDiagnostics(0, NULL);

	TargetOptions to;
	to.Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *pti = TargetInfo::CreateTargetInfo(
			AnnotationCompiler.getDiagnostics(), to);
	AnnotationCompiler.setTarget(pti);

	AnnotationCompiler.createFileManager();
	AnnotationCompiler.createSourceManager(AnnotationCompiler.getFileManager());
	AnnotationCompiler.createPreprocessor();
	AnnotationCompiler.getPreprocessorOpts().UsePredefines = false;
	ASTConsumer *astConsumer = new ASTConsumer();
	AnnotationCompiler.setASTConsumer(astConsumer);

	AnnotationCompiler.createASTContext();
	AnnotationCompiler.createSema(clang::TU_Complete, NULL);

	const FileEntry *pFile = AnnotationCompiler.getFileManager().getFile(
			argv[1]);
	AnnotationCompiler.getSourceManager().createMainFileID(pFile);
	CommentKeeper ck;

	AnnotationCompiler.getPreprocessor().AddCommentHandler(&ck);
	AnnotationCompiler.getPreprocessor().EnterMainSourceFile();
	AnnotationCompiler.getDiagnosticClient().BeginSourceFile(
			AnnotationCompiler.getLangOpts(),
			&AnnotationCompiler.getPreprocessor());
	Parser parser(AnnotationCompiler.getPreprocessor(),
			AnnotationCompiler.getSema(), false /*skipFunctionBodies*/);
	parser.ParseTranslationUnit();
	AnnotationCompiler.getDiagnosticClient().EndSourceFile();

	CompilerInstance Clang;

	Clang.setInvocation(CI.take());

	// Get ready to report problems
	Clang.createDiagnostics(args.size(), &args[0]);
	if (!Clang.hasDiagnostics())
		return 1;

	// Create an action and make the compiler instance carry it out
	llvm::OwningPtr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());
	if (!Clang.ExecuteAction(*Act))
		return 1;

	// Grab the module built by the EmitLLVMOnlyAction
	llvm::Module *module = Act->takeModule();

	// Print all functions in the module

	for_each(module->getGlobalList(), [&](const Value& glob) {
		errs() << glob << endl;
	});
	for_each(module->getFunctionList(),
			[&](const Function& func) {
				errs() << func.getName() << ":\n";
				if(!func.isDeclaration()) {
					for_each(func, [&] (const BasicBlock& bb) {
								for_each(bb, [&] (const Instruction& inst) {
											errs() << "(" << inst.getDebugLoc().getLine() << ',' << inst.getDebugLoc().getCol() << ')' << ' '
											<< inst << endl;
										});
							});
				}
			});

	PassManager pm;

	vector<StringRef> passes2run;

	for_each(args,
			[&passes2run](const StringRef& mes) {
				if( mes.startswith("-pass") ) passes2run.push_back(mes.drop_front(5));
			});

	for_each(passes2run, [](const StringRef& pass) {
		errs() << pass << endl;
	});

	PassNameParser pnm;

	// Initialize passes
	PassRegistry &reg = *PassRegistry::getPassRegistry();
	initializeCore(reg);
	initializeScalarOpts(reg);
	initializeIPO(reg);
	initializeAnalysis(reg);
	initializeIPA(reg);
	initializeTransformUtils(reg);
	initializeInstCombine(reg);
	initializeInstrumentation(reg);
	initializeTarget(reg);

	if(!passes2run.empty()) {
		for_each(passes2run, [&](const StringRef& pass) {
			auto passInfo = reg.getPassInfo(pass);

			errs() << pass << ":"
				   << passInfo->getPassName() << endl;
			pm.add(passInfo->createPass());
		});

		errs() << "Module before passes:" << endl
			   << *module << endl;
		pm.run(*module);
		errs() << "Module after passes:" << endl
			   << *module << endl;

	}
//
//	errs() << "Hi!" << endl;
	return 0;
}
