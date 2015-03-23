/*
 * GlobalVariableLifting.h
 *
 *  Created on: 23 марта 2015 г.
 *      Author: abdullin
 */
#ifndef GLOBALVARIALBLELIFTING_H_
#define GLOBALVARIALBLELIFTING_H_

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/StringMap.h>

namespace borealis {
class GlobalVariableLifting : public llvm::FunctionPass {
private:

    using string_value_map = llvm::StringMap<llvm::Value*>;

    const std::string LOCAL_POSTFIX = ".local";

public:

    static char ID;

    GlobalVariableLifting() : llvm::FunctionPass(ID) {}
    virtual ~GlobalVariableLifting(){}

    virtual bool runOnFunction(llvm::Function& F) override;
};

} /* namespace borealis */

#endif /* GLOBALVARIALBLELIFTING_H_ */
