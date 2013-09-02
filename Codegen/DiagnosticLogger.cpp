/*
 * DiagnosticLogger.cpp
 *
 *  Created on: Aug 27, 2013
 *      Author: belyaev
 */

#include <llvm/ADT/SmallString.h>

#include "Codegen/DiagnosticLogger.h"
#include "Util/locations.h"

namespace borealis {

DiagnosticLogger::DiagnosticLogger():
        clang::DiagnosticConsumer(),
        logging::DelegateLogging() {};

clang::DiagnosticConsumer* DiagnosticLogger::clone(clang::DiagnosticsEngine&) const {
    return new DiagnosticLogger(*this);
}

void DiagnosticLogger::HandleDiagnostic(clang::DiagnosticsEngine::Level Level, const clang::Diagnostic& Info) {

    DiagnosticConsumer::HandleDiagnostic(Level, Info);

    llvm::SmallString<100> Buf;
    Info.FormatDiagnostic(Buf);

    Locus location;
    if(Info.hasSourceManager())
        location = Locus{ Info.getLocation(), Info.getSourceManager() };

    auto infos_     = infos();
    auto warns_     = warns();
    auto errs_      = errs();
    auto criticals_ = criticals();

    auto& stream = (Level == clang::DiagnosticsEngine::Note)    ? (infos_     << "[Compiler message] ") :
                   (Level == clang::DiagnosticsEngine::Warning) ? (warns_     << "[Compiler warning] ") :
                   (Level == clang::DiagnosticsEngine::Error)   ? (errs_      << "[Compiler error] ") :
                   (Level == clang::DiagnosticsEngine::Fatal)   ? (criticals_ << "[Compiler fatal error] ") :
                   util::sayonara<decltype(infos_)&>(__FILE__, __LINE__, __PRETTY_FUNCTION__,
                           "Unknown diag level encountered during clang diag processing");
    if(!location.isUnknown()) stream << location << ": ";
    stream << Buf.c_str() << endl;
}

} /* namespace borealis */
