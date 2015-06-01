/*
 * BasicBlockInformation.cpp
 *
 *  Created on: 22 апр. 2015 г.
 *      Author: kivi
 */

#include "Passes/Misc/Decompiler/BasicBlockInformation.h"
namespace borealis {
namespace decompiler{

void BasicBlockInformation::addBlock(llvm::BasicBlock& bb, BBConfiguration conf) {
    basicBlockInfo[&bb] = conf;
}

llvm::BasicBlock* BasicBlockInformation::getLoopHead(llvm::BasicBlock& bb) {
    auto&& it = basicBlockInfo.find(&bb);
    if(it != basicBlockInfo.end()) return it->second.loopHead;
    else return nullptr;
}

BBPosition BasicBlockInformation::getPosition(llvm::BasicBlock& bb) {
    auto&& it = basicBlockInfo.find(&bb);
    if(it != basicBlockInfo.end()) return it->second.position;
    else return BBPosition::NONE;
}

bool BasicBlockInformation::isVisited(llvm::BasicBlock& bb) {
    auto&& it = basicBlockInfo.find(&bb);
    if(it != basicBlockInfo.end()) return it->second.isVisited;
    else return false;
}

bool BasicBlockInformation::isPrintName(llvm::BasicBlock& bb) {
    auto&& it = basicBlockInfo.find(&bb);
    if(it != basicBlockInfo.end()) return it->second.isPrintName;
    else return false;
}


bool BasicBlockInformation::isInGoto(llvm::BasicBlock& bb) {
    auto&& it = basicBlockInfo.find(&bb);
    if(it != basicBlockInfo.end()) return it->second.isGoto;
    else return false;
}


bool BasicBlockInformation::isContains(llvm::BasicBlock& bb) {
    auto&& it = basicBlockInfo.find(&bb);
    if(it != basicBlockInfo.end()) return true;
    else return false;
}

void BasicBlockInformation::setVisited(llvm::BasicBlock& bb) {
    basicBlockInfo[&bb].isVisited = true;
}

void BasicBlockInformation::setPrintName(llvm::BasicBlock& bb) {
    basicBlockInfo[&bb].isPrintName = true;
}

void BasicBlockInformation::setPosition(llvm::BasicBlock& bb, BBPosition pos) {
    basicBlockInfo[&bb].position = pos;
}

void BasicBlockInformation::setLoopHead(llvm::BasicBlock& bb, llvm::BasicBlock* head) {
    basicBlockInfo[&bb].loopHead = head;
}

void BasicBlockInformation::setInGoto(llvm::BasicBlock& bb)
{
    basicBlockInfo[&bb].isGoto = true;
}

} /* namespace decompiler */
} /* namespace borealis */





