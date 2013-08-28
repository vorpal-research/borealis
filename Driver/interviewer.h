/*
 * interviewer.h
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#ifndef INTERVIEWER_H_
#define INTERVIEWER_H_

#include <clang/Basic/Diagnostic.h>
#include <clang/Driver/Driver.h>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/Path.h>

#include "Config/config.h"
#include "Logging/logger.hpp"
#include "Util/option.hpp"

namespace borealis {
namespace driver {

class interviewer: public borealis::logging::ObjectLevelLogging<interviewer> {
    struct impl;
    std::unique_ptr<impl> pimpl;
public:
    interviewer(
        const std::string& what,
        const std::vector<const char*>& args,
        const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags,
        const borealis::config::StringConfigEntry& config
    );
    ~interviewer();

    enum class status { SUCCESS, FAILURE };

    interviewer::status run() const;
    util::option_ref<const clang::driver::DerivedArgList> getRealArgs() const;
};

} // namespace driver
} // namespace borealis

#endif /* DERIGATO_H_ */
