/*
 * clang_pipeline.h
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#ifndef CLANG_PIPELINE_H_
#define CLANG_PIPELINE_H_

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/Utils.h>

#include <llvm/ADT/IntrusiveRefCntPtr.h>

#include "Logging/logger.hpp"

namespace borealis {
namespace driver {

class clang_pipeline: public logging::ClassLevelLogging<clang_pipeline> {
    struct impl;
    std::unique_ptr<impl> pimpl;

public:
    clang_pipeline(
        const std::string& what,
        const std::vector<const char*>& args,
        const llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine>& diags
    );

    ~clang_pipeline();

    enum class status { SUCCESS, FAILURE };

    std::vector<std::string> getInvocationArgs() const;

    clang::SourceManager& getSourceManager() const;

    void add(clang::FrontendAction& action);

    clang_pipeline::status run();
};

} // namespace driver
} // namespace borealis


#endif /* CLANG_PIPELINE_H_ */
