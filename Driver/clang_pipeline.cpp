/*
 * clang_pipeline.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Arg.h>
#include <clang/Driver/ArgList.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Options.h>
#include <clang/Driver/Tool.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/DiagnosticOptions.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Frontend/Utils.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Linker.h>

#include <fstream>
#include <unordered_map>

#include "Codegen/intrinsics_manager.h"
#include "Driver/clang_pipeline.h"

#include "Factory/Nest.h"
#include "Passes/Transform/MetaInserter.h"
#include "Protobuf/Converter.hpp"

#include "Util/util.hpp"

#include "Util/macros.h"

using namespace clang;

namespace borealis {
namespace driver {

struct clang_pipeline::impl: public DelegateLogging {
    clang::CompilerInstance ci;
    std::unordered_map<std::string, AnnotatedModule::Ptr> fileCache;
    FactoryNest fn;

    impl(clang_pipeline* abs): DelegateLogging(*abs), fn(nullptr) {};

    void compile(const clang::driver::InputArgList& args) {
        std::unique_ptr<CompilerInvocation> CI { new CompilerInvocation() };
        std::vector<const char*> ccArgs;
        ccArgs.reserve(args.getNumInputArgStrings());

        for(auto i = 0U, size = args.getNumInputArgStrings(); i < size; ++i) {
            auto* arg = args.getArgString(i);
            ccArgs.push_back(arg);
        }

        {
            auto info = infos();
            info << "cc " << borealis::util::head(ccArgs);
            for(const auto& arg: borealis::util::tail(ccArgs)) info << " " << arg;
            info << endl;
        }

        ASSERT(
            CompilerInvocation::CreateFromArgs(
              *CI,
              const_cast<const char **>(ccArgs.data()),
              const_cast<const char **>(ccArgs.data()) + ccArgs.size(),
              ci.getDiagnostics()
            ),
            "No CompilerInvocation for clang_pipeline"
        );

        ci.setInvocation(CI.release());

        ASSERT(ci.hasInvocation(), "No CompilerInvocation for clang_pipeline");

        ci.getCodeGenOpts().EmitDeclMetadata = true;
        ci.getCodeGenOpts().DebugInfo = true;
        ci.getDiagnosticOpts().ShowCarets = false;

        clang::EmitLLVMOnlyAction compile_to_llvm{ &llvm::getGlobalContext() };
        borealis::comments::GatherCommentsAction gatherAnnotations;

        ASSERTC(ci.ExecuteAction(compile_to_llvm));
        std::shared_ptr<llvm::Module> module{ compile_to_llvm.takeModule() };

        MetaInserter::liftAllDebugIntrinsics(*module);

        ASSERTC(ci.ExecuteAction(gatherAnnotations));
        AnnotationContainer::Ptr annotations{ new AnnotationContainer(gatherAnnotations, fn.Term) };

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
        std::shared_ptr<llvm::Module> module{ llvm::ParseIRFile(bcfile, diag, llvm::getGlobalContext() ) };
        if(!module) return;

        std::ifstream annoStream(annofile, std::iostream::in | std::iostream::binary);
        if(!annoStream) return;

        AnnotationContainer::ProtoPtr proto { new proto::AnnotationContainer{} };
        proto->ParseFromIstream(&annoStream);
        AnnotationContainer::Ptr annotations = deprotobuffy(fn, *proto);

        if(!annotations) return;

        fileCache[fname] = AnnotatedModule::Ptr{
            new AnnotatedModule{ module, annotations }
        };
    }

    void writeFor(const std::string& fname) {
        std::cerr << "Writing " << fname << std::endl;
        auto bcfile = fname + ".bc";
        auto annofile = fname + ".anno";

        auto annotatedModule = fileCache[fname];

        std::string error;
        llvm::raw_fd_ostream bc_stream(bcfile.c_str(), error);
        llvm::WriteBitcodeToFile(annotatedModule->module.get(), bc_stream);
        errs() << error << endl;

        std::ofstream annoStream(annofile, std::iostream::out | std::iostream::binary);

        AnnotationContainer::ProtoPtr proto = protobuffy(annotatedModule->annotations);
        proto->SerializeToOstream(&annoStream);
    }

    AnnotatedModule::Ptr get(const std::string& name) {
        auto it = fileCache.find(name);
        if(it != fileCache.end()) return it->second;

        readFor(name);
        it = fileCache.find(name);
        if(it != fileCache.end()) return it->second;

        return nullptr;
    }

    void link(const clang::driver::InputArgList& args) {
        std::string moduleName;

        for(const auto& arg: args) errs() << arg->getAsString(args) << endl;

        for(const auto& arg: util::view(
                args.filtered_begin(clang::driver::options::OPT_o),
                                args.filtered_end()
                             )) {
            for(const auto& subarg: arg->getValues()) {
                moduleName = subarg;
            }
        }

        llvm::Linker linker("wrapper", moduleName, llvm::getGlobalContext(), llvm::Linker::PreserveSource);
        AnnotationContainer::Ptr annotations{ new AnnotationContainer() };

        for(const auto& arg: util::view(
                                args.filtered_begin(clang::driver::options::OPT_INPUT),
                                args.filtered_end()
                             )) {
            for(const auto& subarg: arg->getValues()) {
                const auto& am = get(subarg);
                if(!am) {
                    errs() << subarg << ": file or module not found" << endl;
                    continue;
                }

                linker.LinkInModule(am->module.get());

                annotations->mergeIn(am->annotations);
                claim(subarg);
            }
        }

        auto module = util::uniq(linker.releaseModule());
        IntrinsicsManager::getInstance().updateForModule(*module);
        MetaInserter::unliftAllDebugIntrinsics(*module);

        fileCache["<output>"] = AnnotatedModule::Ptr{
            new AnnotatedModule{ std::move(module), std::move(annotations) }
        };
    }

    AnnotatedModule::Ptr result() {
        auto it = fileCache.find("<output>");
        if(it != fileCache.end()) return it->second;
        else return nullptr;
    }

    ~impl () {
        try {
            claim("<output>");
            for(const auto& pfile : fileCache) {
                writeFor(pfile.first);
                // not claim anything, we are in the destructor anywayz
            }
        } catch (...) {
            errs() << "Something awful happened while writing intermediate results" << endl;
        }
    }
};

clang_pipeline::~clang_pipeline() {};

clang_pipeline::clang_pipeline(
        const std::string&,
        const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags):
      pimpl{ new impl{ this } } {
    pimpl->assignLogger(*this);
    pimpl->ci.setDiagnostics(diags.getPtr());
}

void clang_pipeline::invoke(const command& cmd) {
    if(cmd.operation == command::COMPILE) pimpl->compile(*cmd.cl);
    else if (cmd.operation == command::LINK) pimpl->link(*cmd.cl);
}

void clang_pipeline::invoke(const std::vector<command>& cmds) {
    for(const auto& cmd: cmds) {
        invoke(cmd);
    }
}

AnnotatedModule::Ptr clang_pipeline::result() {
    return pimpl->result();
}

} // namespace driver
} // namespace borealis

#include "Util/unmacros.h"
