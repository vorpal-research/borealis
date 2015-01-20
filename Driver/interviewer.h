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
#include "Driver/cl.h"
#include "Logging/logger.hpp"
#include "Util/option.hpp"

namespace borealis {
namespace driver {

struct command {
    enum { COMPILE, LINK, NOP } operation;
    std::shared_ptr<llvm::opt::InputArgList> cl;

    friend std::ostream& operator<<(std::ostream& ost, const command& cmd) {
        switch(cmd.operation) {
        case COMPILE: ost << "cc "; break;
        case LINK: ost << "ld "; break;
        case NOP: ost << "/dev/null "; break;
        default: break;
        }
        ost << cmd.cl;
        return ost;
    }
};

class interviewer: public borealis::logging::DelegateLogging {
    struct impl;
    std::unique_ptr<impl> pimpl;

public:
    enum class status { SUCCESS, FAILURE };

    interviewer(
        const std::string& what,
        const std::vector<const char*>& args,
        const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags,
        const borealis::config::StringConfigEntry& config
    );
    ~interviewer();

    std::vector<command> getCompileCommands();

    interviewer::status run() const;
};

} // namespace driver
} // namespace borealis

#endif /* DERIGATO_H_ */
