/*
 * GlobalVariableLifting.h
 *
 *  Created on: 23 марта 2015 г.
 *      Author: abdullin
 */

#ifndef GLOBALVARIALBLELIFTING_H_
#define GLOBALVARIALBLELIFTING_H_

#include <llvm/ADT/StringMap.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Pass.h>

#include <unordered_map>
#include <unordered_set>

namespace borealis {

class GlobalVariableLifting : public llvm::FunctionPass {

private:

    using string_value_map = std::unordered_map<std::string, llvm::TrackingVH<llvm::Value>>;
    using basic_block_locals_map = std::unordered_map<const llvm::BasicBlock*, string_value_map>;
    using trash_set = std::unordered_set<llvm::Value*>;

    const std::string LIFTED_POSTFIX = ".lifted";

public:

    static char ID;

    GlobalVariableLifting();
    virtual ~GlobalVariableLifting() = default;

    virtual bool runOnFunction(llvm::Function& F) override;

private:

    string_value_map globals;
    basic_block_locals_map locals_begin;
    basic_block_locals_map locals_end;
    trash_set deleted_instructions;
    IntrinsicsManager* IM;

    void init();
    void collectGlobals(llvm::Function& F);
    void setupBasicBlock(llvm::BasicBlock* BB);
    void linkBasicBlock(llvm::BasicBlock* BB);
    void cleanUpSCC(const std::vector<llvm::BasicBlock*>& scc);

    bool replaceLocal(const llvm::BasicBlock* BB, const std::string& name, llvm::Value* value);


};

} /* namespace borealis */

#endif /* GLOBALVARIALBLELIFTING_H_ */
