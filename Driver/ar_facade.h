//
// Created by belyaev on 11/27/15.
//

#ifndef AR_FACADE_H
#define AR_FACADE_H

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/RegionPass.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Option/ArgList.h>
#include <llvm/PassManager.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Program.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/SystemUtils.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Target/TargetMachine.h>

#include <google/protobuf/stubs/common.h>
#include <clang/Driver/Options.h>

#include "Actions/GatherCommentsAction.h"
#include "Config/config.h"
#include "Codegen/DiagnosticLogger.h"
#include "Driver/cl.h"
#include "Driver/clang_pipeline.h"
#include "Driver/gestalt.h"
#include "Driver/interviewer.h"
#include "Driver/llvm_pipeline.h"
#include "Driver/plugin_loader.h"
#include "Logging/logger.hpp"
#include "Passes/Misc/PrinterPasses.h"
#include "Passes/Util/DataProvider.hpp"

#include "Util/util.h"
#include "Driver/cl.h"

namespace borealis {

namespace driver {

class ar_facade {

public:

    int main(int argc, const char** argv) {
        using namespace config;

        GOOGLE_PROTOBUF_VERIFY_VERSION;
        atexit(google::protobuf::ShutdownProtobufLibrary);

        borealis::util::initFilePaths(argv);

        CommandLine args(argc, argv);

        std::string configPath = "wrapper.conf";
        std::string defaultLogIni = "log.ini";

        AppConfiguration::initialize(
            new CommandLineConfigSource{ args.suffixes("---").stlRep() },
            new FileConfigSource{ args.suffixes("---config:").single(util::getFilePathIfExists(configPath).c_str()) }
        );

        config::StringConfigEntry logFile("logging", "ini");

        borealis::logging::configureLoggingFacility(
            util::getFilePathIfExists(logFile.get().getOrElse(defaultLogIni))
        );

        auto argsC = util::view(argv, argv+argc)
            .filter( [](const char* entry){ return llvm::StringRef(entry).endswith_lower(".o") || llvm::StringRef(entry).endswith_lower(".a"); } );

        auto output = argsC.first_or("");
        auto inputs = argsC.drop(1).toVector();

        clang_pipeline clang { "clang" };

        //clang.assignLogger(*this);

        CommandLine pseudoLinker = CommandLine();
        pseudoLinker = pseudoLinker.push_back("-o").push_back(output) + CommandLine(inputs);

        infos() << pseudoLinker << endl;

        driver::command cmd;
        cmd.operation = command::LINK;

        auto optTable = util::uniq(clang::driver::createDriverOptTable());

        unsigned missingArg, missingIndex;
        cmd.cl = decltype(cmd.cl)(optTable->ParseArgs( pseudoLinker.argv(), pseudoLinker.argv() + pseudoLinker.argc(), missingIndex, missingArg ));

        clang.invoke(cmd);

        auto realAr = llvm::sys::FindProgramByName("ar");
        auto realArArgs = CommandLine(argc, argv).nullTerminated();
        return llvm::sys::ExecuteAndWait(realAr, const_cast<const char**>(realArArgs.argv()));
    }

};

} /* namespace driver */

} /* namespace borealis */

#endif //AURORA_SANDBOX_AR_FACADE_HPP
