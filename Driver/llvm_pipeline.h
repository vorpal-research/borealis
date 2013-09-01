/*
 * llvm_pipeline.h
 *
 *  Created on: Aug 20, 2013
 *      Author: belyaev
 */

#ifndef LLVM_PIPELINE_H_
#define LLVM_PIPELINE_H_

#include "Logging/logger.hpp"
#include "Passes/Util/DataProvider.hpp"

namespace borealis {
namespace driver {

class llvm_pipeline: public logging::DelegateLogging {
    struct impl;
    std::unique_ptr<impl> pimpl;

    void addPass(llvm::Pass*);

public:
    enum class status { SUCCESS, FAILURE };

    llvm_pipeline(const std::shared_ptr<llvm::Module>& m);
    ~llvm_pipeline();
    
#include "Util/macros.h"
    template<class T, class = GUARD(std::is_base_of<llvm::Pass, T>::value)>
    void add(T* pass) {
        addPass(pass);
    }
#include "Util/unmacros.h"

    void add(const std::string& pname);

    template<class T>
    void add(const T& value) {
        addPass(provideAsPass(&value));
    }

    status run();
};

} // namespace driver
} // namespace borealis

#endif /* LLVM_PIPELINE_H_ */
