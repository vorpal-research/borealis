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

#include "Driver/clang_pipeline.h"

#include "Factory/Nest.h"
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
        std::cerr << "cc ";

        std::unique_ptr<CompilerInvocation> CI { new CompilerInvocation() };
        std::vector<const char*> ccArgs;
        ccArgs.reserve(args.getNumInputArgStrings());
        for(auto i = 0U, size = args.getNumInputArgStrings(); i < size; ++i) {
            auto* arg = args.getArgString(i);
            ccArgs.push_back(arg);
            std::cerr << arg << " ";
        }
        std::cerr << std::endl;

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

        ASSERTC(ci.ExecuteAction(gatherAnnotations));
        AnnotationContainer::Ptr annotations{ new AnnotationContainer(gatherAnnotations, fn.Term) };

        std::cerr << "Gathered module" << ci.getFrontendOpts().OutputFile << ": "
                << std::endl << util::toString(*module) << std::endl;

        fileCache[ci.getFrontendOpts().OutputFile] = AnnotatedModule::Ptr{
            new AnnotatedModule{ module, annotations }
        };

        std::cerr << "Gathered module" << ci.getFrontendOpts().OutputFile << ": "
                << std::endl << util::toString(*get(ci.getFrontendOpts().OutputFile)->module) << std::endl;

        std::cerr << "The file cache:" << std::endl;
        for(const auto& file: fileCache) {
            std::cerr << file.first << " : " << util::toString(*file.second->module) << std::endl;
        }

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
        std::cerr << error << std::endl;

        std::ofstream annoStream(annofile, std::iostream::out | std::iostream::binary);

        AnnotationContainer::ProtoPtr proto = protobuffy(annotatedModule->annotations);
        proto->SerializeToOstream(&annoStream);
    }

    AnnotatedModule::Ptr get(const std::string& name) {
        auto it = fileCache.find(name);
        if(it != fileCache.end()) return it->second;

        std::cerr << "FUCKA" << std::endl;

        readFor(name);
        it = fileCache.find(name);
        if(it != fileCache.end()) return it->second;

        return nullptr;
    }

    void link(const clang::driver::InputArgList& args) {
        std::cerr << "Linker called!" << std::endl;
        std::string moduleName;

        for(const auto& arg: args) std::cerr << arg->getAsString(args) << std::endl;

        for(const auto& arg: util::view(
                args.filtered_begin(clang::driver::options::OPT_o),
                                args.filtered_end()
                             )) {
            for(const auto& subarg: arg->getValues()) {
                moduleName = subarg;
            }
        }

        llvm::Linker linker("wrapper", moduleName, llvm::getGlobalContext(), llvm::Linker::DestroySource);
        AnnotationContainer::Ptr annotations{ new AnnotationContainer() };

        for(const auto& arg: util::view(
                                args.filtered_begin(clang::driver::options::OPT_INPUT),
                                args.filtered_end()
                             )) {
            std::cerr << "ARG" << std::endl;
            for(const auto& subarg: arg->getValues()) {
                std::cerr << subarg << std::endl;

                const auto& am = get(subarg);
                std::cerr << am.get() << std::endl;

                if(!am) {
                    std::cerr << subarg << ": file or module not found" << std::endl;
                    continue;
                }


                std::cerr << util::toString(*am->module) << std::endl;

                linker.LinkInModule(am->module.get());

                errs() << *linker.getModule();

                annotations->mergeIn(am->annotations);
                claim(subarg);
            }
        }

        fileCache["<output>"] = AnnotatedModule::Ptr{
            new AnnotatedModule{ util::uniq(linker.releaseModule()), std::move(annotations) }
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
