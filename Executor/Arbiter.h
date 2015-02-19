/*
 * Arbiter.h
 *
 *  Created on: Feb 5, 2015
 *      Author: belyaev
 */

#ifndef EXECUTOR_ARBITER_H_
#define EXECUTOR_ARBITER_H_

#include <memory>

#include <llvm/IR/Value.h>
#include <llvm/ExecutionEngine/GenericValue.h>

namespace borealis {

class Arbiter {
public:
    virtual llvm::GenericValue map(llvm::Value* val) = 0;
    virtual ~Arbiter();

    using Ptr = std::shared_ptr<Arbiter>;
};

} /* namespace borealis */

#endif /* EXECUTOR_ARBITER_H_ */
