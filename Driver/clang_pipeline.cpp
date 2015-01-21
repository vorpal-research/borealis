/*
 * clang_pipeline.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Options.h>
#include <clang/Driver/Tool.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Frontend/Utils.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Host.h>
#include <llvm/IR/TypeBuilder.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Option/Arg.h>
#include <llvm/Option/ArgList.h>

#include <fstream>
#include <unordered_map>

#include "Actions/VariableInfoFinder.h"
#include "Codegen/intrinsics_manager.h"
#include "Codegen/llvm.h"
#include "Driver/clang_pipeline.h"
#include "Factory/Nest.h"
#include "Passes/Transform/MetaInserter.h"
#include "Protobuf/Converter.hpp"
#include "Util/locations.h"
#include "Util/util.hpp"

#include "Util/macros.h"

using namespace clang;

namespace borealis {
namespace driver {

constexpr auto linkerOutputFile = "..<output>..";

static void postProcessClangGeneratedModule(clang::SourceManager& SM, llvm::Module& M) {
    auto globalsMD = M.getNamedMetadata("clang.global.decl.ptrs");
    if(!globalsMD) return;

    auto ourGlobals = M.getOrInsertNamedMetadata("bor.global.decls");
    auto& ctx = M.getContext();

    for(auto i = 0U; i < globalsMD->getNumOperands(); ++i) {
        auto cacheLine = globalsMD->getOperand(i);
        if(cacheLine->getNumOperands() != 2
        || cacheLine->getOperand(0) == nullptr
        || cacheLine->getOperand(1) == nullptr
        || !llvm::isa<llvm::GlobalValue>(cacheLine->getOperand(0))
        || !llvm::isa<llvm::ConstantInt>(cacheLine->getOperand(1))
        ) continue;

        // TODO: remake that so it uses standard llvm location format?

        // this is generally fucked up, but it is the way of doing it
        auto* clangDecl =
            static_cast<clang::VarDecl*>(
                reinterpret_cast<void*>(llvm::cast<llvm::ConstantInt>(
                    cacheLine->getOperand(1))->getZExtValue()
                )
            );

        Locus llc { clangDecl->getLocStart(), SM };

        auto mline = llvm::ConstantInt::get(
            llvm::TypeBuilder<llvm::types::i<32>, true>::get(ctx), llc.loc.line
        );
        auto mcol = llvm::ConstantInt::get(
            llvm::TypeBuilder<llvm::types::i<32>, true>::get(ctx), llc.loc.col
        );

         llvm::Value* arr[] = {
            cacheLine->getOperand(0),
            llvm::MDString::get(ctx, clangDecl->getName()),
            mcol,
            mline,
            llvm::MDString::get(ctx, llc.filename)
        };
        ourGlobals->addOperand(llvm::MDNode::get(ctx, arr));
    }

    MetaInserter::liftAllDebugIntrinsics(M);
}

struct clang_pipeline::impl: public DelegateLogging {
    clang::CompilerInstance ci;
    std::unordered_map<std::string, AnnotatedModule::Ptr> fileCache;
    FactoryNest fn;

    impl(clang_pipeline* abs): DelegateLogging(*abs), fn(nullptr) {};

    void compile(const llvm::opt::InputArgList& args) {
        std::unique_ptr<CompilerInvocation> CI { new CompilerInvocation() };
        std::vector<const char*> ccArgs;
        ccArgs.reserve(args.getNumInputArgStrings());

        for (auto i = 0U, size = args.getNumInputArgStrings(); i < size; ++i) {
            auto* arg = args.getArgString(i);
            ccArgs.push_back(arg);
        }

        {
            auto info = infos();
            info << "cc " << borealis::util::head(ccArgs);
            for (const auto& arg: borealis::util::tail(ccArgs)) info << " " << arg;
            info << endl;
        }

        ASSERT(
            CompilerInvocation::CreateFromArgs(
                *CI,
                const_cast<const char**>(ccArgs.data()),
                const_cast<const char**>(ccArgs.data()) + ccArgs.size(),
                ci.getDiagnostics()
            ),
            "No CompilerInvocation for clang_pipeline"
        );

        ci.setInvocation(CI.release());
        ASSERT(ci.hasInvocation(), "No CompilerInvocation for clang_pipeline");

        ci.getCodeGenOpts().EmitDeclMetadata = true;
        ci.getCodeGenOpts().setDebugInfo(clang::CodeGenOptions::FullDebugInfo);
        ci.getDiagnosticOpts().ShowCarets = false;

        clang::EmitLLVMOnlyAction compile_to_llvm{ &llvm::getGlobalContext() };
        borealis::comments::GatherCommentsAction gatherAnnotations;

        ASSERTC(ci.ExecuteAction(compile_to_llvm));
        std::shared_ptr<llvm::Module> module{ compile_to_llvm.takeModule() };

        postProcessClangGeneratedModule(ci.getSourceManager(), *module);

        ASSERTC(ci.ExecuteAction(gatherAnnotations));
        AnnotationContainer::Ptr annotations{ new AnnotationContainer(gatherAnnotations, fn.Term) };

        borealis::VariableInfoFinder vif;
        ASSERTC(ci.ExecuteAction(vif));

        fileCache[ci.getFrontendOpts().OutputFile] = AnnotatedModule::Ptr{
            new AnnotatedModule{ module, annotations }
        };
    }

    void claim(const std::string& fname) {
        fileCache.erase(fname);
    }

    void readFor(const std::string& fname) {
        auto bcfile = fname + ".bc";
        auto annofile = fname + ".anno";

        llvm::SMDiagnostic diag;
        std::shared_ptr<llvm::Module> module{ llvm::ParseIRFile(bcfile, diag, llvm::getGlobalContext()) };
        if (!module) return;

        std::ifstream annoStream(annofile, std::iostream::in | std::iostream::binary);
        if (!annoStream) return;

        AnnotationContainer::ProtoPtr proto{ new proto::AnnotationContainer{} };
        proto->ParseFromIstream(&annoStream);

        AnnotationContainer::Ptr annotations = deprotobuffy(fn, *proto);
        if (!annotations) return;

        fileCache[fname] = AnnotatedModule::Ptr{
            new AnnotatedModule{ module, annotations }
        };
    }

    void writeFor(const std::string& fname) {
        auto bcfile = fname + ".bc";
        auto annofile = fname + ".anno";

        auto annotatedModule = fileCache[fname];
        ASSERTC(annotatedModule != nullptr);

        std::string error;
        llvm::raw_fd_ostream bc_stream(bcfile.c_str(), error, llvm::sys::fs::F_Text | llvm::sys::fs::F_RW);
        llvm::WriteBitcodeToFile(annotatedModule->module.get(), bc_stream);
        errs() << error << endl;

        AnnotationContainer::ProtoPtr proto = protobuffy(annotatedModule->annotations);
        std::ofstream annoStream(annofile, std::iostream::out | std::iostream::binary);
        proto->SerializeToOstream(&annoStream);
    }

    AnnotatedModule::Ptr get(const std::string& name) {
        auto it = fileCache.find(name);
        if (it != fileCache.end()) return it->second;

        readFor(name);
        it = fileCache.find(name);
        if (it != fileCache.end()) return it->second;

        return nullptr;
    }

    void link(const llvm::opt::InputArgList& args) {
        std::string moduleName;

        {
            auto info = infos();
            info << "ld " << borealis::util::head(args)->getAsString(args);
            for (const auto& arg: borealis::util::tail(args)) info << " " << arg->getAsString(args);
            info << endl;
        }

        for (const auto& arg: util::view(
            args.filtered_begin(clang::driver::options::OPT_o),
            args.filtered_end()
        )) {
            for (const auto& subarg: arg->getValues()) {
                moduleName = subarg;
                break;
            }
            break;
        }

        auto module = util::uniq(new llvm::Module(moduleName, llvm::getGlobalContext()));
        llvm::Linker linker(module.get(), false);
        AnnotationContainer::Ptr annotations{ new AnnotationContainer() };

        for (const auto& arg: util::view(
            args.filtered_begin(clang::driver::options::OPT_INPUT),
            args.filtered_end()
        )) {
            for (const auto& subarg: arg->getValues()) {
                const auto& am = get(subarg);
                if (!am) {
                    warns() << subarg << ": file or module not found" << endl;
                    continue;
                }

                std::string err;
                if(linker.linkInModule(am->module.get(), llvm::Linker::DestroySource, &err)) {
                    errs() << "Errors during linking: " << err << endl;
                }
                annotations->mergeIn(am->annotations);
                claim(subarg);
            }
        }

        IntrinsicsManager::getInstance().updateForModule(*module);
        MetaInserter::unliftAllDebugIntrinsics(*module);

        fileCache[linkerOutputFile] = AnnotatedModule::Ptr{
            new AnnotatedModule{ std::move(module), std::move(annotations) }
        };
    }

    AnnotatedModule::Ptr result() {
        auto it = fileCache.find(linkerOutputFile);
        if (it != fileCache.end()) return it->second;
        else return nullptr;
    }

    ~impl () {
        try {
            claim(linkerOutputFile);
            for (const auto& pfile : fileCache) {
                writeFor(pfile.first);
                // do not claim anything, we are in the destructor anywayz
            }
        } catch (...) {
            errs() << "Something awful happened while writing intermediate results" << endl;
        }
    }
};

clang_pipeline::~clang_pipeline() {};

clang_pipeline::clang_pipeline(
    const std::string&,
    llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags):
        pimpl{ new impl{ this } } {
    pimpl->assignLogger(*this);
    pimpl->ci.setDiagnostics(diags.get());
}

void clang_pipeline::invoke(const command& cmd) {
    if (cmd.operation == command::COMPILE) pimpl->compile(*cmd.cl);
    else if (cmd.operation == command::LINK) pimpl->link(*cmd.cl);
}

void clang_pipeline::invoke(const std::vector<command>& cmds) {
    for (const auto& cmd: cmds) {
        invoke(cmd);
    }
}

AnnotatedModule::Ptr clang_pipeline::result() {
    return pimpl->result();
}

} // namespace driver
} // namespace borealis

#include "Util/unmacros.h"
