/*
 * BasicBlockInformation.h
 *
 *  Created on: 22 апреля 2015 г.
 *      Author: kivi
 */

#ifndef BASICBLOCKINFORMATION_H_
#define BASICBLOCKINFORMATION_H_

#include <unordered_map>
#include <llvm/IR/BasicBlock.h>

namespace borealis {
namespace decompiler {

enum BBPosition {NONE = 0, IN_LOOP = 1, WHILE_BEGIN = 2, WHILE_END = 3, DO_WHILE_BEGIN = 4, DO_WHILE_END = 5};

struct BBConfiguration {
    bool isPrintName;
    bool isVisited;
    BBPosition position;
    llvm::BasicBlock* loopHead;
    bool isGoto;
};


class BasicBlockInformation {
private:
    std::unordered_map<llvm::BasicBlock*, BBConfiguration> basicBlockInfo;

public:

    BasicBlockInformation() : basicBlockInfo() {};

    void addBlock(llvm::BasicBlock& bb, BBConfiguration conf);

    llvm::BasicBlock* getLoopHead(llvm::BasicBlock& bb);
    BBPosition getPosition(llvm::BasicBlock& bb);
    bool isVisited(llvm::BasicBlock& bb);
    bool isPrintName(llvm::BasicBlock& bb);
    bool isContains(llvm::BasicBlock& bb);
    bool isInGoto(llvm::BasicBlock& bb);

    void setLoopHead(llvm::BasicBlock& bb, llvm::BasicBlock* st);
    void setVisited(llvm::BasicBlock& bb);
    void setPrintName(llvm::BasicBlock& bb);
    void setPosition(llvm::BasicBlock& bb, BBPosition pos);
    void setInGoto(llvm::BasicBlock& bb);

    inline void clear() {basicBlockInfo.clear();}

};

} /* namespace decompiler */
} /* namespace borealis */

#endif /* BASICBLOCKINFORMATION_H_ */
