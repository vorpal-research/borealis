/*
 * DiagnosticLogger.h
 *
 *  Created on: Aug 27, 2013
 *      Author: belyaev
 */

#ifndef DIAGNOSTICLOGGER_H_
#define DIAGNOSTICLOGGER_H_

#include <vector>

#include <clang/Basic/Diagnostic.h>

#include "Logging/logger.hpp"

namespace borealis {

class DiagnosticLogger : public virtual clang::DiagnosticConsumer, public logging::DelegateLogging {
public:
    DiagnosticLogger();
    DiagnosticLogger(const DiagnosticLogger&) = default;
    template<class T>
    DiagnosticLogger(const T& value): DelegateLogging(value){};

    virtual clang::DiagnosticConsumer *clone(clang::DiagnosticsEngine &Diags) const;

    virtual void HandleDiagnostic(clang::DiagnosticsEngine::Level Level, const clang::Diagnostic &Info);
};

} /* namespace borealis */
#endif /* DIAGNOSTICLOGGER_H_ */
