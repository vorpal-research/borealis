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

    switch(Level) {
    case clang::DiagnosticsEngine::Note:
        infos()     << "[Compiler message] "     << location << ": " << Buf.c_str() << endl; break;
    case clang::DiagnosticsEngine::Warning:
        warns()     << "[Compiler warning] "     << location << ": " << Buf.c_str() << endl; break;
    case clang::DiagnosticsEngine::Error:
        errs()      << "[Compiler error] "       << location << ": " << Buf.c_str() << endl; break;
    case clang::DiagnosticsEngine::Fatal:
        criticals() << "[Compiler fatal error] " << location << ": " << Buf.c_str() << endl; break;
    default:
#include "Util/macros.h"
        BYE_BYE_VOID("Unknown diag level encountered during clang diag processing");
#include "Util/unmacros.h"
    }
}

} /* namespace borealis */
